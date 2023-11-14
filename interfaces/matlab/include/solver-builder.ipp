// This is a stripped-down version of src/alpaqa/src/driver

#include <alpaqa/config/config.hpp>
#include <alpaqa/inner/directions/panoc/anderson.hpp>
#include <alpaqa/inner/directions/panoc/lbfgs.hpp>
#include <alpaqa/inner/directions/panoc/structured-lbfgs.hpp>
#include <alpaqa/inner/directions/pantr/newton-tr.hpp>
#include <alpaqa/inner/panoc.hpp>
#include <alpaqa/inner/pantr.hpp>
#include <alpaqa/inner/zerofpr.hpp>
#include <alpaqa/outer/alm.hpp>
#include <alpaqa/params/json.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>
#include <alpaqa/util/string-util.hpp>
#include <nlohmann/json.hpp>

#include <alpaqa-mex-types.hpp>

namespace alpaqa::mex {

USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);

using solver_runner_func =
    std::function<SolverResults(alpaqa::TypeErasedProblem<config_t> &problem,
                                crvec x0, crvec y0, std::ostream &os)>;
using solver_builder_func =
    std::function<solver_runner_func(std::string_view, const Options &opts)>;

template <class T>
decltype(auto) set_params(T &t, std::string_view prefix, const Options &opts) {
    try {
        if (auto j = opts.find(prefix); j != opts.end())
            alpaqa::params::set_param(t, *j);
    } catch (alpaqa::params::invalid_json_param &e) {
        throw std::invalid_argument(
            "Error in params at '" + std::string(prefix) +
            alpaqa::util::join_quote(std::views::reverse(e.backtrace),
                                     {.sep         = "",
                                      .empty       = "",
                                      .quote_left  = ".",
                                      .quote_right = ""}) +
            "': " + e.what());
    } catch (nlohmann::json::exception &e) {
        throw std::invalid_argument(std::string("Error in params: ") +
                                    e.what());
    }
}

template <class InnerSolver>
auto make_alm_solver(InnerSolver &&inner_solver, const Options &opts) {
    // Settings for the ALM solver
    using ALMSolver = alpaqa::ALMSolver<InnerSolver>;
    typename ALMSolver::Params alm_param;
    set_params(alm_param, "alm", opts);
    return ALMSolver{alm_param, std::forward<InnerSolver>(inner_solver)};
}

template <class InnerSolver>
auto make_inner_solver(const Options &opts) {
    // Settings for the solver
    typename InnerSolver::Params solver_param;
    set_params(solver_param, "solver", opts);

    if constexpr (requires { typename InnerSolver::Direction; }) {
        // Settings for the direction provider
        using Direction = typename InnerSolver::Direction;
        typename Direction::DirectionParams dir_param;
        typename Direction::AcceleratorParams accel_param;
        set_params(dir_param, "dir", opts);
        set_params(accel_param, "accel", opts);
        return InnerSolver{
            solver_param,
            Direction{{
                .accelerator = accel_param,
                .direction   = dir_param,
            }},
        };
    } else {
        return InnerSolver{solver_param};
    }
}

template <class Solver>
SolverResults run_alm_solver(typename Solver::Problem &problem, crvec x0,
                             crvec y0, Solver &solver, std::ostream &os) {

    solver.os = &os;

    // Initial guess
    vec x = x0, y = y0;
    if (x.size() == 0)
        x = vec::Zero(problem.get_n());
    if (y.size() == 0)
        y = vec::Zero(problem.get_m());
    // Final penalties
    vec Σ = vec::Constant(problem.get_m(), alpaqa::NaN<config_t>);

    // Solve the problem
    auto s = solver(problem, x, y, Σ);

    solver.os = &std::cout;

    // Results and statistics
    nlohmann::json stats      = nlohmann::json::object();
    stats["status"]           = enum_name(s.status);
    stats["success"]          = s.status == alpaqa::SolverStatus::Converged;
    stats["solver"]           = solver.get_name();
    stats["delta"]            = s.δ;
    stats["epsilon"]          = s.ε;
    stats["norm_penalty"]     = s.norm_penalty;
    stats["outer_iterations"] = s.outer_iterations;
    alpaqa::params::get_param(s.elapsed_time, stats["elapsed_time"]);
    auto &inner = stats["inner"] = nlohmann::json::object();
    inner["iterations"]          = s.inner.iterations;
    real_t final_γ = 0, final_h = 0;
    if constexpr (requires { s.inner.final_γ; })
        final_γ = s.inner.final_γ;
    inner["final_gamma"] = final_γ;
    if constexpr (requires { s.inner.final_h; })
        final_h = s.inner.final_h;
    inner["final_h"] = final_h;
    if constexpr (requires { s.inner.linesearch_failures; })
        inner["linesearch_failures"] =
            static_cast<index_t>(s.inner.linesearch_failures);
    if constexpr (requires { s.inner.linesearch_backtracks; })
        inner["linesearch_backtracks"] =
            static_cast<index_t>(s.inner.linesearch_backtracks);
    if constexpr (requires { s.inner.stepsize_backtracks; })
        inner["stepsize_backtracks"] =
            static_cast<index_t>(s.inner.stepsize_backtracks);
    if constexpr (requires { s.inner.lbfgs_failures; })
        inner["lbfgs_failures"] = static_cast<index_t>(s.inner.lbfgs_failures);
    if constexpr (requires { s.inner.lbfgs_rejected; })
        inner["lbfgs_rejected"] = static_cast<index_t>(s.inner.lbfgs_rejected);
    if constexpr (requires { s.inner.accelerated_step_rejected; })
        inner["accelerated_step_rejected"] =
            static_cast<index_t>(s.inner.accelerated_step_rejected);
    if constexpr (requires { s.inner.direction_failures; })
        inner["direction_failures"] =
            static_cast<index_t>(s.inner.direction_failures);
    if constexpr (requires { s.inner.direction_update_rejected; })
        inner["direction_update_rejected"] =
            static_cast<index_t>(s.inner.direction_update_rejected);
    return {
        .x     = std::vector(x.begin(), x.end()),
        .y     = std::vector(y.begin(), y.end()),
        .stats = std::move(stats),
    };
}

template <class T>
struct tag_t {};

template <template <class Direction> class Solver>
solver_runner_func make_panoc_like_driver(std::string_view direction,
                                          const Options &opts) {
    auto builder = []<class Direction>(tag_t<Direction>) {
        return [](std::string_view, const Options &opts) -> solver_runner_func {
            using Problem     = typename Solver<Direction>::Problem;
            auto inner_solver = make_inner_solver<Solver<Direction>>(opts);
            auto solver       = make_alm_solver(std::move(inner_solver), opts);
            return [solver{std::move(solver)}](
                       Problem &problem, crvec x0, crvec y0,
                       std::ostream &os) mutable -> SolverResults {
                return run_alm_solver(problem, std::move(x0), std::move(y0),
                                      solver, os);
            };
        };
    };
    std::map<std::string_view, solver_builder_func> builders{
        {"lbfgs", //
         builder(tag_t<alpaqa::LBFGSDirection<config_t>>())},
        {"anderson", //
         builder(tag_t<alpaqa::AndersonDirection<config_t>>())},
        {"struclbfgs", //
         builder(tag_t<alpaqa::StructuredLBFGSDirection<config_t>>())},
    };
    if (direction.empty())
        direction = "lbfgs";
    auto builder_it = builders.find(direction);
    if (builder_it != builders.end())
        return builder_it->second(direction, opts);
    else
        throw std::invalid_argument(
            "Unknown direction '" + std::string(direction) + "'\n" +
            "  Available directions: " +
            alpaqa::util::join(std::views::keys(builders)));
}

solver_runner_func make_panoc_driver(std::string_view direction,
                                     const Options &opts) {
    return make_panoc_like_driver<alpaqa::PANOCSolver>(direction, opts);
}

solver_runner_func make_zerofpr_driver(std::string_view direction,
                                       const Options &opts) {
    return make_panoc_like_driver<alpaqa::ZeroFPRSolver>(direction, opts);
}

template <template <class Direction> class Solver>
solver_runner_func make_pantr_like_solver(std::string_view direction,
                                          const Options &opts) {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    auto builder = []<class Direction>(tag_t<Direction>) {
        return [](std::string_view, const Options &opts) -> solver_runner_func {
            using Problem     = typename Solver<Direction>::Problem;
            auto inner_solver = make_inner_solver<Solver<Direction>>(opts);
            auto solver       = make_alm_solver(std::move(inner_solver), opts);
            return [solver{std::move(solver)}](
                       Problem &problem, crvec x0, crvec y0,
                       std::ostream &os) mutable -> SolverResults {
                return run_alm_solver(problem, std::move(x0), std::move(y0),
                                      solver, os);
            };
        };
    };
    std::map<std::string_view, solver_builder_func> builders{
        {"newtontr", //
         builder(tag_t<alpaqa::NewtonTRDirection<config_t>>())},
    };
    if (direction.empty())
        direction = "newtontr";
    auto builder_it = builders.find(direction);
    if (builder_it != builders.end())
        return builder_it->second(direction, opts);
    else
        throw std::invalid_argument(
            "Unknown direction '" + std::string(direction) + "'\n" +
            "  Available directions: " +
            alpaqa::util::join(std::views::keys(builders)));
}

solver_runner_func make_pantr_driver(std::string_view direction,
                                     const Options &opts) {
    return make_pantr_like_solver<alpaqa::PANTRSolver>(direction, opts);
}

auto get_solver_builder(std::string_view method,
                        [[maybe_unused]] const Options &opts) {
    std::string direction;
    if (method.empty())
        method = "panoc.struclbfgs";
    std::tie(method, direction) = alpaqa::util::split(method, ".");
    // Dictionary of available solver builders
    std::map<std::string_view, solver_builder_func> solvers{
        {"panoc", make_panoc_driver},
        {"zerofpr", make_zerofpr_driver},
        {"pantr", make_pantr_driver},
        // TODO
        // {"lbfgsb", make_lbfgsb_driver},
        // {"fista", make_fista_driver},
        // {"ipopt", make_ipopt_driver},
        // {"qpalm", make_qpalm_driver},
    };
    // Find the selected solver builder
    auto solver_it = solvers.find(method);
    if (solver_it == solvers.end())
        throw std::invalid_argument(
            "Unknown solver '" + std::string(method) + "'\n" +
            "  Available solvers: " +
            alpaqa::util::join(std::views::keys(solvers)));
    return std::make_tuple(std::move(solver_it->second), direction);
}

} // namespace alpaqa::mex
