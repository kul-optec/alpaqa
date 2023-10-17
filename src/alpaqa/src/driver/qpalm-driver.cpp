#ifdef WITH_QPALM

#include <alpaqa/qpalm/qpalm-adapter.hpp>
#include <qpalm/constants.h>

#include <cstdarg>
#include <stdexcept>
#include <string>

#include "results.hpp"
#include "solver-driver.hpp"

namespace {

USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);

std::ostream *qpalm_os = nullptr;
int print_wrap(const char *fmt, ...) LADEL_ATTR_PRINTF_LIKE;
int print_wrap_noop(const char *, ...) LADEL_ATTR_PRINTF_LIKE;

void compress_multipliers_bounds(length_t n, length_t m,
                                 const alpaqa::OwningQPALMData &qp,
                                 crvec multipliers_bounds, rvec y) {
    for (index_t col = 0; col < n; ++col) {
        // Loop over the constraint matrix
        auto outer = qp.A->p[col];
        auto nnz   = qp.A->p[col + 1] - outer;
        if (nnz == 0) // No constraints for this variable
            continue;
        auto row = qp.A->i[outer];
        if (row + m >= y.size()) // Only top rows are bound constraints
            continue;
        // Negative multipliers correspond to the lower bound (first half of the
        // vector), positive multipliers to the upper bound (second half)
        auto val_lb = multipliers_bounds(col);
        auto val_ub = multipliers_bounds(col + n);
        y(row)      = val_lb < 0 ? val_lb : val_ub;
    }
}

void expand_multipliers_bounds(length_t n, length_t m,
                               const alpaqa::OwningQPALMData &qp, crvec sol_y,
                               rvec multipliers_bounds) {
    for (index_t col = 0; col < n; ++col) {
        // Loop over the constraint matrix
        auto outer = qp.A->p[col];
        auto nnz   = qp.A->p[col + 1] - outer;
        if (nnz == 0) // No constraints for this variable
            continue;
        auto row = qp.A->i[outer];
        if (row + m >= sol_y.size()) // Only top rows are bound constraints
            continue;
        // Negative multipliers correspond to the lower bound (first half of the
        // vector), positive multipliers to the upper bound (second half)
        auto value                       = sol_y(row);
        index_t offset                   = value > 0 ? n : 0;
        multipliers_bounds(col + offset) = value;
    }
}

SolverResults run_qpalm_solver(auto &problem, const qpalm::Settings &settings,
                               std::ostream &os, unsigned N_exp) {

    // Set up output stream
    qpalm_os       = &os;
    auto old_print = ladel_set_print_config_printf(&print_wrap);
    struct PrintRestorer {
        printf_sig *print;
        ~PrintRestorer() { ladel_set_print_config_printf(print); }
    } restore_print{old_print};

    // Dimensions
    length_t n = problem.problem.get_n(), m = problem.problem.get_m();

    // Adapt problem
    auto qp = alpaqa::build_qpalm_problem(problem.problem);
    qpalm::Solver solver{&qp, settings};

    // Initial guess
    vec initial_guess_mult;
    if (auto sz = problem.initial_guess_x.size(); sz != n)
        throw std::invalid_argument("Invalid size for initial_guess_x (got " +
                                    std::to_string(sz) + ", expected " +
                                    std::to_string(n) + ")");
    if (auto sz = problem.initial_guess_y.size(); sz != m)
        throw std::invalid_argument("Invalid size for initial_guess_y (got " +
                                    std::to_string(sz) + ", expected " +
                                    std::to_string(m) + ")");
    if (auto sz = problem.initial_guess_w.size(); sz > 0) {
        if (sz != n * 2)
            throw std::invalid_argument(
                "Invalid size for initial_guess_w (got " + std::to_string(sz) +
                ", expected " + std::to_string(n * 2) + ")");
        initial_guess_mult.resize(static_cast<length_t>(qp.m));
        compress_multipliers_bounds(n, m, qp, problem.initial_guess_w,
                                    initial_guess_mult);
        initial_guess_mult.bottomRows(m) = problem.initial_guess_y;
    }
    auto warm_start = [&] {
        problem.initial_guess_w.size() > 0
            ? solver.warm_start(problem.initial_guess_x, initial_guess_mult)
            : solver.warm_start(problem.initial_guess_x, std::nullopt);
    };

    // Solve the problem
    auto t0 = std::chrono::steady_clock::now();
    warm_start();
    solver.solve();
    auto t1    = std::chrono::steady_clock::now();
    auto evals = *problem.evaluations;
    auto info  = solver.get_info();
    vec sol_x = solver.get_solution().x, sol_y = solver.get_solution().y;

    // Solve the problems again to average runtimes
    using ns          = std::chrono::nanoseconds;
    auto avg_duration = duration_cast<ns>(t1 - t0);
    ladel_set_print_config_printf(&print_wrap_noop);
    os.setstate(std::ios_base::badbit);
    for (unsigned i = 0; i < N_exp; ++i) {
        auto t0 = std::chrono::steady_clock::now();
        warm_start();
        solver.solve();
        auto t1 = std::chrono::steady_clock::now();
        avg_duration += duration_cast<ns>(t1 - t0);
    }
    os.clear();
    avg_duration /= (N_exp + 1);

    // Results
    SolverResults results{
        .status             = info.status,
        .success            = info.status_val == QPALM_SOLVED,
        .evals              = evals,
        .duration           = avg_duration,
        .solver             = "QPALM",
        .h                  = 0,
        .δ                  = info.dua_res_norm,
        .ε                  = info.pri_res_norm,
        .γ                  = 0,
        .Σ                  = 0,
        .solution           = sol_x,
        .multipliers        = sol_y.bottomRows(m),
        .multipliers_bounds = vec::Zero(n * 2), // see bleow
        .penalties          = vec::Zero(n),
        .outer_iter         = info.iter_out,
        .inner_iter         = info.iter,
        .extra              = {{"dua2_res_norm", info.dua2_res_norm}},
    };
    // Expand the multipliers for the bounds constraints again
    expand_multipliers_bounds(n, m, qp, sol_y, results.multipliers_bounds);
    return results;
}

int print_wrap(const char *fmt, ...) {
    static std::vector<char> buffer(1024);
    std::va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    int needed = vsnprintf(buffer.data(), buffer.size(), fmt, args);
    va_end(args);
    // Error occurred
    if (needed < 0) {
        // ignore and return
    }
    // Buffer was too small
    else if (auto buf_needed = static_cast<size_t>(needed) + 1;
             buf_needed > buffer.size()) {
        buffer.resize(buf_needed);
        va_start(args2, fmt);
        needed = vsnprintf(buffer.data(), buffer.size(), fmt, args2);
        va_end(args2);
    }
    if (needed >= 0) {
        assert(qpalm_os);
        std::string_view out{buffer.data(), static_cast<size_t>(needed)};
        *qpalm_os << out;
    }
    return needed;
}

int print_wrap_noop(const char *, ...) { return 0; }

auto get_qpalm_settings(Options &opts) {
    qpalm::Settings settings;
    settings.eps_abs = 1e-8;
    settings.eps_rel = 1e-8;
    set_params(settings, "solver", opts);
    return settings;
}

template <class LoadedProblem>
SharedSolverWrapper make_qpalm_drive_impl(std::string_view direction,
                                          Options &opts) {
    if (!direction.empty())
        throw std::invalid_argument(
            "QPALM solver does not support any directions");
    auto settings  = get_qpalm_settings(opts);
    unsigned N_exp = 0;
    set_params(N_exp, "num_exp", opts);
    return std::make_shared<SolverWrapper>(
        [settings, N_exp](LoadedProblem &problem,
                          std::ostream &os) mutable -> SolverResults {
            return run_qpalm_solver(problem, settings, os, N_exp);
        });
}

} // namespace

SharedSolverWrapper make_qpalm_driver(std::string_view direction,
                                      Options &opts) {
    static constexpr bool valid_config =
        std::is_same_v<LoadedProblem::config_t, alpaqa::EigenConfigd>;
    if constexpr (valid_config)
        return make_qpalm_drive_impl<LoadedProblem>(direction, opts);
    else
        throw std::invalid_argument(
            "QPALM solver only supports double precision");
}

#else

#include "solver-driver.hpp"

SharedSolverWrapper make_qpalm_driver(std::string_view, Options &) {
    throw std::invalid_argument(
        "This version of alpaqa was compiled without QPALM support.");
}

#endif
