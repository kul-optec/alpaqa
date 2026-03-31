#include <alpaqa-jl/export.h>

#include "julia-types.hpp"

#include <alpaqa/casadi/CasADiProblem.hpp>
#include <alpaqa/driver/alm-driver.hpp>
#include <alpaqa/driver/alpaqa-driver.hpp>
#include <alpaqa/driver/cancel.hpp>
#include <alpaqa/inner/inner-solve-options.hpp>
#include <alpaqa/params/options.hpp>
#include <alpaqa/problem-loader/problem-loader.hpp>
#include <alpaqa/problem/problem-with-counters.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>
#include <alpaqa/util/check-dim.hpp>
#include <guanaqo/demangled-typename.hpp>
#include <guanaqo/eigen/span.hpp>
#include <guanaqo/linalg/sparsity-conversions.hpp>
#include <guanaqo/linalg/sparsity.hpp>

#include <alpaqa/implementation/outer/alm.tpp>

#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tuple>

namespace alpaqa::jl {

USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);

/// Wraps an alpaqa problem with specific values for the multipliers and
/// penalties, and provides the (sub)problem functions required by the Julia
/// inner solver. A Julia-compatible pointer to this wrapped problem can be
/// created using get_julia_problem.
/// Non-owning. Not thread safe.
struct SubProblem {
    crvec Σ;
    crvec y;
    const TypeErasedProblem<config_t> &problem;
    mutable vec work_n{problem.get_num_variables()},
        zero_n{vec::Zero(problem.get_num_variables())},
        work_m{problem.get_num_constraints()};
    mutable std::vector<double> work_H{};

    static void check_dim(length_t actual, length_t expected, std::string msg) {
        if (actual != expected) {
            msg += "\n(should be ";
            msg += std::to_string(expected);
            msg += ", got ";
            msg += std::to_string(actual);
            msg += ")";
            throw std::invalid_argument(msg);
        }
    }

    double value_and_gradient(const double *x, size_t nx,
                              double *grad_f) const {
        auto inx = static_cast<length_t>(nx);
        check_dim(inx, problem.get_num_variables(),
                  "value_and_gradient argument dimension mismatch");
        return problem.eval_augmented_lagrangian_and_gradient(
            cmvec{x, inx}, y, Σ, mvec{grad_f, inx}, work_n, work_m);
    }

    double prox(double gamma, const double *x, size_t nx,
                double *prox_h) const {
        auto inx = static_cast<length_t>(nx);
        check_dim(inx, problem.get_num_variables(),
                  "prox argument dimension mismatch");
        return problem.eval_proximal_gradient_step(gamma, cmvec{x, inx}, zero_n,
                                                   mvec{prox_h, inx}, work_n);
    }

    void hvp(const double *x, size_t nx, const double *v, double *Hv) const {
        auto inx = static_cast<length_t>(nx);
        check_dim(inx, problem.get_num_variables(),
                  "hvp argument dimension mismatch");
        return problem.eval_augmented_lagrangian_hessian_product(
            cmvec{x, inx}, y, Σ, 1, cmvec{v, inx}, mvec{Hv, inx});
    }

    void jprox(double gamma, const double *x, size_t nx, real_t *J_diag) const {
        auto inx = static_cast<length_t>(nx);
        check_dim(inx, problem.get_num_variables(),
                  "jprox argument dimension mismatch");
        return problem.eval_prox_jacobian_diag(gamma, cmvec{x, inx},
                                               mvec{J_diag, inx});
    }

    void hess(const double *x, size_t nx, double *H) const {
        auto inx = static_cast<length_t>(nx);
        check_dim(inx, problem.get_num_variables(),
                  "hess argument dimension mismatch");
        auto sp = problem.get_augmented_lagrangian_hessian_sparsity();
        sparsity::SparsityConverter<Sparsity, sparsity::Dense> conv{sp};
        work_H = conv.convert_values_into(
            std::span<double>(H, nx * nx),
            [&](std::span<double> H) {
                return problem.eval_augmented_lagrangian_hessian(
                    cmvec{x, inx}, y, Σ, 1, guanaqo::as_vec(H));
            },
            std::move(work_H));
    }

    double value(const double *x, size_t nx) const {
        auto inx = static_cast<length_t>(nx);
        check_dim(inx, problem.get_num_variables(),
                  "value argument dimension mismatch");
        return problem.eval_augmented_lagrangian(cmvec{x, inx}, y, Σ, work_m);
    }

    void gradient(const double *x, size_t nx, double *grad_f) const {
        auto inx = static_cast<length_t>(nx);
        check_dim(inx, problem.get_num_variables(),
                  "gradient argument dimension mismatch");
        problem.eval_augmented_lagrangian_gradient(
            cmvec{x, inx}, y, Σ, mvec{grad_f, inx}, work_n, work_m);
    }

    template <auto M>
    static auto memfun() {
        return guanaqo::type_erased_wrapped<SubProblem, M>();
    }

    julia::SubProblem get_julia_problem() const {
        return {
            .data               = this,
            .value_and_gradient = memfun<&SubProblem::value_and_gradient>(),
            .prox               = memfun<&SubProblem::prox>(),
            .hvp                = memfun<&SubProblem::hvp>(),
            .jprox              = memfun<&SubProblem::jprox>(),
            .hess               = memfun<&SubProblem::hess>(),
            .value              = memfun<&SubProblem::value>(),
            .gradient           = memfun<&SubProblem::gradient>(),
        };
    }
};

/// Thin wrapper around the Julia callable that solves inner problems.
struct InnerSolverFunction {
    julia::InnerSolverFunction callable;
    julia::JuliaInnerSolverResult operator()(const SubProblem &problem, rvec x,
                                             double tol) const {
        return callable.function(callable.context, problem.get_julia_problem(),
                                 x.data(), static_cast<size_t>(x.size()), tol);
    }
};

/// Inner solver type that can be plugged into the alpaqa::ALMSolver class.
/// It simply calls the given Julia function to solve the inner problems.
struct JuliaInnerSolver {
    USING_ALPAQA_CONFIG(jl::config_t);
    using Problem      = TypeErasedProblem<config_t>;
    using SolveOptions = InnerSolveOptions<config_t>;

    InnerSolverFunction jl_inner_solver;

    struct Stats {
        SolverStatus status = SolverStatus::Busy;
        double ε            = std::numeric_limits<double>::infinity();
        unsigned iterations = 0;
        std::chrono::nanoseconds elapsed_time{};
    };

    Stats operator()(const Problem &problem, const SolveOptions &opts, rvec x,
                     rvec y, crvec Σ, rvec err_z);

    void stop() { /* TODO */ }
    [[nodiscard]] std::string get_name() const {
        return guanaqo::demangled_typename(typeid(*this));
    }
};

auto JuliaInnerSolver::operator()(const Problem &problem,
                                  const SolveOptions &opts, rvec x, rvec y,
                                  crvec Σ, rvec err_z) -> Stats {
    jl::SubProblem jl_prob{.Σ = Σ, .y = y, .problem = problem};
    // Call inner solver
    using clock = std::chrono::steady_clock;
    auto t0     = clock::now();
    auto result = jl_inner_solver(jl_prob, x, opts.tolerance);
    auto t1     = clock::now();
    // Update Lagrange multipliers
    auto &ŷ     = jl_prob.work_m;
    std::ignore = problem.eval_augmented_lagrangian(x, y, Σ, ŷ);
    if (err_z.size() > 0)
        err_z = (ŷ - y).cwiseQuotient(Σ);
    y = ŷ;
    return Stats{
        .status       = result.success ? SolverStatus::Converged //
                                       : SolverStatus::Exception,
        .ε            = result.achieved_tol,
        .iterations   = static_cast<unsigned>(result.num_iter),
        .elapsed_time = duration_cast<std::chrono::nanoseconds>(t1 - t0),
    };
}

} // namespace alpaqa::jl
namespace alpaqa {

/// This template specialization tells the ALMSolver how to accumulate the
/// statistics returned by the inner solver.
template <>
struct InnerStatsAccumulator<jl::JuliaInnerSolver::Stats> {
    std::chrono::nanoseconds elapsed_time{};
    unsigned iterations = 0;

    friend InnerStatsAccumulator &
    operator+=(InnerStatsAccumulator &acc,
               const jl::JuliaInnerSolver::Stats &s) {
        acc.iterations += s.iterations;
        acc.elapsed_time += s.elapsed_time;
        return acc;
    }
};

} // namespace alpaqa
namespace alpaqa::jl {

/// Load a problem from the given file path, with the given type (e.g. "cs" for
/// CasADi problems), and with the given argument list.
std::unique_ptr<LoadedProblem> load_problem(const std::string &type,
                                            const fs::path &path, size_t nargs,
                                            const char *const args[]) {
    alpaqa::Options options{static_cast<int>(nargs), args};
    return std::make_unique<LoadedProblem>(
        alpaqa::load_problem(type, path, options));
}

/// Create an ALMSolver with the given Julia function as the inner solver, and
/// solve the given problem.
julia::SolverReturn run_alm(LoadedProblem &problem,
                            const InnerSolverFunction &jl_inner_solver, rvec x,
                            rvec y, size_t nargs, const char *const args[]) {
    alpaqa::Options options{static_cast<int>(nargs), args};
    ALMParams<config_t> alm_params{.print_interval = 1};
    set_params(alm_params, "alm", options);

    // Check options
    auto used       = options.used();
    auto unused_opt = std::ranges::find(used, 0);
    auto unused_idx = static_cast<size_t>(unused_opt - used.begin());
    if (unused_opt != used.end())
        throw std::invalid_argument("Invalid option: " +
                                    std::string(options.options()[unused_idx]));

    ALMSolver<JuliaInnerSolver> solver{
        alm_params,
        {.jl_inner_solver = jl_inner_solver},
    };
    auto stats = solver(problem.problem, x, y);
    return {
        .success        = stats.status == SolverStatus::Converged,
        .evaluations    = // clang-format off
            {
                .projecting_difference_constraints = problem.evaluations->projecting_difference_constraints,
                .projection_multipliers = problem.evaluations->projection_multipliers,
                .proximal_gradient_step = problem.evaluations->proximal_gradient_step,
                .inactive_indices_res_lna = problem.evaluations->inactive_indices_res_lna,
                .prox_jacobian_diag = problem.evaluations->prox_jacobian_diag,
                .nonsmooth_objective = problem.evaluations->nonsmooth_objective,
                .objective = problem.evaluations->objective,
                .objective_gradient = problem.evaluations->objective_gradient,
                .objective_and_gradient = problem.evaluations->objective_and_gradient,
                .objective_and_constraints = problem.evaluations->objective_and_constraints,
                .objective_gradient_and_constraints_gradient_product = problem.evaluations->objective_gradient_and_constraints_gradient_product,
                .constraints = problem.evaluations->constraints,
                .constraints_gradient_product = problem.evaluations->constraints_gradient_product,
                .grad_gi = problem.evaluations->grad_gi,
                .constraints_jacobian = problem.evaluations->constraints_jacobian,
                .lagrangian_gradient = problem.evaluations->lagrangian_gradient,
                .lagrangian_hessian_product = problem.evaluations->lagrangian_hessian_product,
                .lagrangian_hessian = problem.evaluations->lagrangian_hessian,
                .augmented_lagrangian_hessian_product = problem.evaluations->augmented_lagrangian_hessian_product,
                .augmented_lagrangian_hessian = problem.evaluations->augmented_lagrangian_hessian,
                .augmented_lagrangian = problem.evaluations->augmented_lagrangian,
                .augmented_lagrangian_gradient = problem.evaluations->augmented_lagrangian_gradient,
                .augmented_lagrangian_and_gradient = problem.evaluations->augmented_lagrangian_and_gradient,
            }, // clang-format on
        .num_inner_iter = stats.inner.iterations,
        .num_outer_iter = stats.outer_iterations,
        .num_outer_fail = stats.inner_convergence_failures,
    };
}

jl::julia::DriverReturn
run_alpaqa(std::string_view prob_path, std::string_view prob_type,
           LoadedProblem *problem,
           const alpaqa::jl::julia::InnerSolverFunction *jl_inner_solver,
           rvec x, rvec y, size_t argc, const char *const argv[]) try {
    using namespace alpaqa::driver;
    std::span args{argv, argc};
    alpaqa::Options opts{static_cast<int>(argc), argv};

    // Check arguments
    if (problem && !prob_path.empty())
        throw std::invalid_argument(
            "run_alpaqa: cannot have both problem path and problem instance");

    // Check where to write the output to
    std::ofstream out_fstream;
    std::ostream &os = get_output_stream(opts, out_fstream, std::cout);

    // Check which solver to use
    SolverBuilders builders{"jl"};
    if (jl_inner_solver) {
        builders.register_solver_builder(
            "jl",
            [&](std::string_view, alpaqa::Options &) -> SharedSolverWrapper {
                JuliaInnerSolver inner_solver{*jl_inner_solver};
                auto solver    = make_alm_solver(std::move(inner_solver), opts);
                unsigned N_exp = 0;
                set_params(N_exp, "num_exp", opts);
                auto run = [solver{std::move(solver)},
                            N_exp](alpaqa::LoadedProblem &problem,
                                   std::ostream &os) mutable -> SolverResults {
                    auto cancel = attach_cancellation(solver);
                    return run_alm_solver(problem, solver, os, N_exp);
                };
                return std::make_shared<SolverWrapper>(std::move(run));
            });
    }
    auto [solver_builder, direction] = builders.get_solver_builder(opts);

    // Check output paths
    fs::path sol_output_dir = get_output_paths(opts);

    // Build solver
    auto solver = solver_builder(direction, opts);

    // Load problem
    std::optional<LoadedProblem> loaded_problem;
    if (!problem) {
        os << "Loading " << prob_path << " ..." << std::endl;
        problem = &loaded_problem.emplace(
            alpaqa::load_problem(prob_type, prob_path, opts));
    }

    // Initial guess x
    if (x.size() > 0) {
        util::check_dim_msg(x, problem->problem.get_num_variables(),
                            "Invalid initial guess size x");
        problem->initial_guess_x = x;
    }
    // Initial guess y
    if (y.size() > 0) {
        util::check_dim_msg(y, problem->problem.get_num_constraints(),
                            "Invalid initial guess size y");
        problem->initial_guess_y = y;
    }

    // Print problem information
    bool show_funcs = false;
    set_params(show_funcs, "show_funcs", opts);
    print_problem_description(os, *problem, show_funcs);
    os << std::endl;

    // Check options
    auto used       = opts.used();
    auto unused_opt = std::ranges::find(used, 0);
    auto unused_idx = static_cast<size_t>(unused_opt - used.begin());
    if (unused_opt != used.end())
        throw std::invalid_argument("Unused option: " +
                                    std::string(opts.options()[unused_idx]));

    // Solve
    auto solver_results = solver->run(*problem, os);

    // Return solution
    if (x.size() > 0)
        x = solver_results.solution;
    if (y.size() > 0)
        y = solver_results.multipliers;

    // Compute more statistics
    real_t f     = problem->problem.eval_objective(solver_results.solution);
    auto kkt_err = alpaqa::compute_kkt_error(
        problem->problem, solver_results.solution, solver_results.multipliers);
    BenchmarkResults results{
        .problem          = *problem,
        .solver_results   = solver_results,
        .objective        = f + solver_results.h,
        .smooth_objective = f,
        .error            = kkt_err,
        .options          = opts.options(),
        .timestamp        = timestamp_ms<std::chrono::system_clock>().count(),
    };
    jl::julia::DriverReturn ret{
        .success     = results.solver_results.success,
        .evaluations = // clang-format off
            {
                .projecting_difference_constraints = results.solver_results.evals.projecting_difference_constraints,
                .projection_multipliers = results.solver_results.evals.projection_multipliers,
                .proximal_gradient_step = results.solver_results.evals.proximal_gradient_step,
                .inactive_indices_res_lna = results.solver_results.evals.inactive_indices_res_lna,
                .prox_jacobian_diag = results.solver_results.evals.prox_jacobian_diag,
                .nonsmooth_objective = results.solver_results.evals.nonsmooth_objective,
                .objective = results.solver_results.evals.objective,
                .objective_gradient = results.solver_results.evals.objective_gradient,
                .objective_and_gradient = results.solver_results.evals.objective_and_gradient,
                .objective_and_constraints = results.solver_results.evals.objective_and_constraints,
                .objective_gradient_and_constraints_gradient_product = results.solver_results.evals.objective_gradient_and_constraints_gradient_product,
                .constraints = results.solver_results.evals.constraints,
                .constraints_gradient_product = results.solver_results.evals.constraints_gradient_product,
                .grad_gi = results.solver_results.evals.grad_gi,
                .constraints_jacobian = results.solver_results.evals.constraints_jacobian,
                .lagrangian_gradient = results.solver_results.evals.lagrangian_gradient,
                .lagrangian_hessian_product = results.solver_results.evals.lagrangian_hessian_product,
                .lagrangian_hessian = results.solver_results.evals.lagrangian_hessian,
                .augmented_lagrangian_hessian_product = results.solver_results.evals.augmented_lagrangian_hessian_product,
                .augmented_lagrangian_hessian = results.solver_results.evals.augmented_lagrangian_hessian,
                .augmented_lagrangian = results.solver_results.evals.augmented_lagrangian,
                .augmented_lagrangian_gradient = results.solver_results.evals.augmented_lagrangian_gradient,
                .augmented_lagrangian_and_gradient = results.solver_results.evals.augmented_lagrangian_and_gradient,
            },
        .num_inner_iter = static_cast<unsigned>(results.solver_results.inner_iter),
        .num_outer_iter = static_cast<unsigned>(results.solver_results.outer_iter),
        .output_file_id = nullptr,
        .error          = nullptr,
                       // clang-format on
    };

    // Print results
    print_results(os, results);

    // Store solution
    if (!sol_output_dir.empty()) {
        auto fid =
            store_solution(sol_output_dir, os, results, *solver, opts, args);
        ret.output_file_id = ::strndup(fid.c_str(), fid.size());
    }
    return ret;
} catch (std::exception &e) {
    auto msg = guanaqo::demangled_typename(typeid(e)) + ":\n  " + e.what();
    return {.error = ::strndup(msg.c_str(), msg.size())};
}

auto &as_casadi_problem(TypeErasedProblem<config_t> &problem) {
    using CsProblem   = alpaqa::CasADiProblem<config_t>;
    using CntProblem  = alpaqa::ProblemWithCounters<CsProblem>;
    auto &cnt_problem = problem.as<CntProblem>();
    return cnt_problem.problem;
}

size_t param_size_casadi(LoadedProblem &problem) {
    auto &cs_problem = as_casadi_problem(problem.problem);
    return static_cast<size_t>(cs_problem.param.size());
}

void get_param_casadi(LoadedProblem &problem, rvec param) {
    auto &cs_problem = as_casadi_problem(problem.problem);
    if (cs_problem.param.size() != param.size())
        throw std::invalid_argument("Incorrect parameter size (expected " +
                                    std::to_string(cs_problem.param.size()) +
                                    ", but got " +
                                    std::to_string(param.size()) + ")");
    param = cs_problem.param;
}

void set_param_casadi(LoadedProblem &problem, crvec param) {
    auto &cs_problem = as_casadi_problem(problem.problem);
    if (cs_problem.param.size() != param.size())
        throw std::invalid_argument("Incorrect parameter size (expected " +
                                    std::to_string(cs_problem.param.size()) +
                                    ", but got " +
                                    std::to_string(param.size()) + ")");
    cs_problem.param = param;
}

casadi::Function &dynamics_casadi(LoadedProblem &problem) {
    casadi::Function *dynamics = nullptr;
#if !ALPAQA_WITH_EXTERNAL_CASADI
    auto &cs_problem = as_casadi_problem(problem.problem);
    dynamics         = cs_problem.extra_function(0);
#endif
    if (!dynamics)
        throw std::runtime_error("No dynamics available for this problem");
    if (dynamics->n_in() != 2)
        throw std::runtime_error("Unexpected number of arguments for dynamics");
    if (dynamics->n_out() != 1)
        throw std::runtime_error("Unexpected number of outputs for dynamics");
    if (1 != dynamics->size2_in(0))
        throw std::runtime_error("Dynamics first argument should be a vector");
    if (1 != dynamics->size2_in(1))
        throw std::runtime_error("Dynamics second argument should be a vector");
    if (1 != dynamics->size2_out(0))
        throw std::runtime_error("Dynamics output should be a vector");
    if (dynamics->size1_in(0) != dynamics->size1_out(0))
        throw std::runtime_error("Dynamics state dimension mismatch");
    return *dynamics;
}

void ocp_simulate_casadi(LoadedProblem &problem, crvec x, crvec u,
                         rvec x_next) {
    auto &dynamics = dynamics_casadi(problem);
    if (x.size() != dynamics.size1_in(0))
        throw std::invalid_argument("Incorrect state dimension for dynamics");
    if (u.size() != dynamics.size1_in(1))
        throw std::invalid_argument("Incorrect input dimension for dynamics");
    if (x_next.size() != dynamics.size1_out(0))
        throw std::invalid_argument(
            "Incorrect next state dimension for dynamics");
    const double *args[]{x.data(), u.data()}; // NOLINT(*-c-arrays)
    double *out[]{x_next.data()};             // NOLINT(*-c-arrays)
    dynamics(args, out);
}

size_t ocp_nx_casadi(LoadedProblem &problem) {
    auto &dynamics = dynamics_casadi(problem);
    return static_cast<size_t>(dynamics.size1_in(0));
}

size_t ocp_nu_casadi(LoadedProblem &problem) {
    auto &dynamics = dynamics_casadi(problem);
    return static_cast<size_t>(dynamics.size1_in(1));
}

} // namespace alpaqa::jl

// Below are the public functions that are exported from the DLL. They use the
// C calling convention and linkage, so they can be called directly from Julia
// using `ccall`.

extern "C" ALPAQA_JL_EXPORT alpaqa::LoadedProblem *
alpaqa_jl_load_problem(const char *type, const char *path, size_t nargs,
                       const char *const args[]) {
    return alpaqa::jl::load_problem(type, path, nargs, args).release();
}

extern "C" ALPAQA_JL_EXPORT void
alpaqa_jl_free_problem(alpaqa::LoadedProblem *problem) {
    std::unique_ptr<alpaqa::LoadedProblem>{problem};
}

extern "C" ALPAQA_JL_EXPORT size_t
alpaqa_jl_num_variables(alpaqa::LoadedProblem *problem) {
    return static_cast<size_t>(problem->problem.get_num_variables());
}

extern "C" ALPAQA_JL_EXPORT size_t
alpaqa_jl_num_constraints(alpaqa::LoadedProblem *problem) {
    return static_cast<size_t>(problem->problem.get_num_constraints());
}

extern "C" ALPAQA_JL_EXPORT void
alpaqa_jl_initial_guess(alpaqa::LoadedProblem *problem, double *x0, size_t n,
                        double *y0, size_t m) {
    using namespace alpaqa::jl;
    if (n != static_cast<size_t>(problem->initial_guess_x.size()))
        throw std::invalid_argument("incorrect size for initial_guess_x");
    if (m != static_cast<size_t>(problem->initial_guess_y.size()))
        throw std::invalid_argument("incorrect size for initial_guess_y");
    mvec{x0, static_cast<length_t>(n)} = problem->initial_guess_x;
    mvec{y0, static_cast<length_t>(m)} = problem->initial_guess_y;
}

extern "C" ALPAQA_JL_EXPORT void
alpaqa_jl_bounds(alpaqa::LoadedProblem *problem, double *xl, double *xu,
                 size_t n) {
    using namespace alpaqa::jl;
    if (n != static_cast<size_t>(problem->problem.get_num_variables()))
        throw std::invalid_argument("incorrect size for bounds xl, xu");
    const auto &C                      = problem->problem.get_variable_bounds();
    mvec{xl, static_cast<length_t>(n)} = C.lower;
    mvec{xu, static_cast<length_t>(n)} = C.upper;
}

extern "C" ALPAQA_JL_EXPORT void
alpaqa_jl_lagrangian_gradient(alpaqa::LoadedProblem *problem, const double *x,
                              size_t n, const double *y, size_t m,
                              double *grad) {
    using namespace alpaqa::jl;
    if (n != static_cast<size_t>(problem->problem.get_num_variables()))
        throw std::invalid_argument("incorrect size for x");
    if (m != static_cast<size_t>(problem->problem.get_num_constraints()))
        throw std::invalid_argument("incorrect size for y");
    vec w{static_cast<length_t>(n)};
    problem->problem.eval_lagrangian_gradient(
        cmvec{x, static_cast<length_t>(n)}, cmvec{y, static_cast<length_t>(m)},
        mvec{grad, static_cast<length_t>(n)}, w);
}

extern "C" ALPAQA_JL_EXPORT void
alpaqa_jl_reset_counters_problem(alpaqa::LoadedProblem *problem) {
    problem->evaluations->reset();
}

extern "C" ALPAQA_JL_EXPORT void
alpaqa_jl_describe_problem(alpaqa::LoadedProblem *problem) {
    alpaqa::print_problem_description(std::cout, *problem);
}

extern "C" ALPAQA_JL_EXPORT void
alpaqa_jl_print_counters_problem(alpaqa::LoadedProblem *problem) {
    std::cout << *problem->evaluations;
}

extern "C" ALPAQA_JL_EXPORT alpaqa::jl::julia::SolverReturn
alpaqa_jl_run_alm(alpaqa::LoadedProblem *problem,
                  const alpaqa::jl::julia::InnerSolverFunction *jl_inner_solver,
                  double *x, size_t nx, double *y, size_t ny, size_t nargs,
                  const char *args[]) {
    using namespace alpaqa::jl;
    return run_alm(*problem, {*jl_inner_solver},
                   mvec{x, static_cast<length_t>(nx)},
                   mvec{y, static_cast<length_t>(ny)}, nargs, args);
}

extern "C" ALPAQA_JL_EXPORT alpaqa::jl::julia::DriverReturn
alpaqa_jl_run_alpaqa(
    const char *problem_path, const char *problem_type,
    alpaqa::LoadedProblem *problem,
    const alpaqa::jl::julia::InnerSolverFunction *jl_inner_solver, double *x,
    size_t nx, double *y, size_t ny, size_t nargs, const char *const args[]) {
    using namespace alpaqa::jl;
    return run_alpaqa(problem_path, problem_type, problem, jl_inner_solver,
                      mvec{x, static_cast<length_t>(nx)},
                      mvec{y, static_cast<length_t>(ny)}, nargs, args);
}

extern "C" ALPAQA_JL_EXPORT size_t alpaqa_jl_param_size_casadi(
    alpaqa::LoadedProblem *problem, double *value, size_t size) {
    using namespace alpaqa::jl;
    return param_size_casadi(*problem);
}

extern "C" ALPAQA_JL_EXPORT void
alpaqa_jl_get_param_casadi(alpaqa::LoadedProblem *problem, double *value,
                           size_t size) {
    using namespace alpaqa::jl;
    get_param_casadi(*problem, mvec{value, static_cast<length_t>(size)});
}

extern "C" ALPAQA_JL_EXPORT void
alpaqa_jl_set_param_casadi(alpaqa::LoadedProblem *problem, const double *value,
                           size_t size) {
    using namespace alpaqa::jl;
    set_param_casadi(*problem, cmvec{value, static_cast<length_t>(size)});
}

extern "C" ALPAQA_JL_EXPORT void
alpaqa_jl_ocp_simulate_casadi(alpaqa::LoadedProblem *problem, const double *x,
                              size_t nx, const double *u, size_t nu,
                              double *x_next, size_t nx_next) {
    using namespace alpaqa::jl;
    ocp_simulate_casadi(*problem, cmvec{x, static_cast<length_t>(nx)},
                        cmvec{u, static_cast<length_t>(nu)},
                        mvec{x_next, static_cast<length_t>(nx_next)});
}

extern "C" ALPAQA_JL_EXPORT size_t
alpaqa_jl_ocp_nx_casadi(alpaqa::LoadedProblem *problem) {
    using namespace alpaqa::jl;
    return ocp_nx_casadi(*problem);
}

extern "C" ALPAQA_JL_EXPORT size_t
alpaqa_jl_ocp_nu_casadi(alpaqa::LoadedProblem *problem) {
    using namespace alpaqa::jl;
    return ocp_nu_casadi(*problem);
}
