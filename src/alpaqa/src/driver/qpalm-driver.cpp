#ifdef ALPAQA_WITH_QPALM

#include <alpaqa/qpalm/qpalm-adapter.hpp>
#include <alpaqa/util/lin-constr-converter.hpp>
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

void compress_multipliers_bounds(const alpaqa::sets::Box<config_t> &C, rvec y,
                                 crvec multipliers_bounds) {
    using Conv    = alpaqa::LinConstrConverter<config_t, index_t, index_t>;
    index_t shift = 0;
    for (index_t i = 0; i < C.lowerbound.size(); ++i)
        if (Conv::is_bound(C, i))
            y(shift++) = multipliers_bounds(i);
}

void expand_multipliers_bounds(const alpaqa::sets::Box<config_t> &C, crvec y,
                               rvec multipliers_bounds) {
    using Conv    = alpaqa::LinConstrConverter<config_t, index_t, index_t>;
    index_t shift = 0;
    for (index_t i = 0; i < C.lowerbound.size(); ++i)
        if (Conv::is_bound(C, i))
            multipliers_bounds(i) = y(shift++);
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

    // Adapt problem
    auto qp = alpaqa::build_qpalm_problem(problem.problem);
    qpalm::Solver solver{&qp, settings};

    // Dimensions
    length_t n = problem.problem.get_num_variables(),
             m = problem.problem.get_num_constraints();
    [[maybe_unused]] length_t num_bounds = static_cast<length_t>(qp.m) - m;

    // Initial guess
    vec initial_guess_mult;
    if (auto sz = problem.initial_guess_x.size(); sz != n)
        throw std::invalid_argument(
            "Invalid size for initial_guess_x (expected " + std::to_string(n) +
            ", but got " + std::to_string(sz) + ")");
    if (auto sz = problem.initial_guess_y.size(); sz != m)
        throw std::invalid_argument(
            "Invalid size for initial_guess_y (expected " + std::to_string(m) +
            ", but got " + std::to_string(sz) + ")");
    if (auto sz = problem.initial_guess_w.size(); sz > 0) {
        if (sz != n)
            throw std::invalid_argument(
                "Invalid size for initial_guess_w (expected " +
                std::to_string(n) + ", but got " + std::to_string(sz) + ")");
        initial_guess_mult.resize(static_cast<length_t>(qp.m));
        if (problem.problem.provides_get_box_variables())
            compress_multipliers_bounds(problem.problem.get_box_variables(),
                                        initial_guess_mult,
                                        problem.initial_guess_w);
        else
            assert(num_bounds == 0 && initial_guess_mult.size() == m);
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
    auto t1   = std::chrono::steady_clock::now();
    auto info = solver.get_info();
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
    auto evals = *problem.evaluations;

    // Results
    SolverResults results{
        .status             = info.status,
        .success            = info.status_val == QPALM_SOLVED,
        .evals              = evals,
        .duration           = avg_duration,
        .solver             = "QPALM",
        .h                  = 0,
        .δ                  = info.pri_res_norm,
        .ε                  = info.dua_res_norm,
        .γ                  = 0,
        .Σ                  = 0,
        .solution           = sol_x,
        .multipliers        = sol_y.bottomRows(m),
        .multipliers_bounds = vec::Zero(n), // see bleow
        .penalties          = vec::Zero(n),
        .outer_iter         = info.iter_out,
        .inner_iter         = info.iter,
        .extra              = {{"dua2_res_norm", info.dua2_res_norm}},
    };
    // Expand the multipliers for the bounds constraints again
    if (problem.problem.provides_get_box_variables())
        expand_multipliers_bounds(problem.problem.get_box_variables(), sol_y,
                                  results.multipliers_bounds);
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
