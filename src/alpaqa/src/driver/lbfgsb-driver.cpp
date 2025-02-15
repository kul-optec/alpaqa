#if ALPAQA_WITH_LBFGSB

#include <alpaqa/implementation/outer/alm.tpp>
#include <alpaqa/lbfgsb/lbfgsb-adapter.hpp>
#include <alpaqa/params/options.hpp>

#include <alpaqa/driver/alm-driver.hpp>
#include <alpaqa/driver/cancel.hpp>
#include <alpaqa/driver/solver-driver.hpp>
#include "extra-stats.hpp"
#include "lbfgsb-driver.hpp"

namespace alpaqa::driver {
namespace {

using InnerLBFGSBSolver = lbfgsb::LBFGSBSolver;

auto make_inner_lbfgsb_solver(Options &opts) {
    // Settings for the solver
    InnerLBFGSBSolver::Params solver_param;
    solver_param.max_iter       = 50'000;
    solver_param.print_interval = 0;
    solver_param.stop_crit      = PANOCStopCrit::ProjGradUnitNorm;
    set_params(solver_param, "solver", opts);
    return InnerLBFGSBSolver{solver_param};
}

template <class LoadedProblem>
SharedSolverWrapper make_lbfgsb_driver_impl(std::string_view direction,
                                            Options &opts) {
    if (!direction.empty())
        throw std::invalid_argument(
            "L-BFGS-B solver does not support any directions");
    using collector_t = AlpaqaSolverStatsCollector<config_t>;
    std::shared_ptr<collector_t> collector;
    auto inner_solver = make_inner_lbfgsb_solver(opts);
    bool extra_stats  = false;
    set_params(extra_stats, "extra_stats", opts);
    if (extra_stats) {
        collector = std::make_shared<collector_t>();
        inner_solver.set_progress_callback(
            [collector](const auto &progress_info) {
                collector->update_iter(progress_info);
            });
    }
    auto solver    = make_alm_solver(std::move(inner_solver), opts);
    unsigned N_exp = 0;
    set_params(N_exp, "num_exp", opts);
    auto run = [solver{std::move(solver)},
                N_exp](LoadedProblem &problem,
                       std::ostream &os) mutable -> SolverResults {
        auto cancel = attach_cancellation(solver);
        return run_alm_solver(problem, solver, os, N_exp);
    };
    return std::make_shared<AlpaqaSolverWrapperStats<config_t>>(
        std::move(run), std::move(collector));
}

} // namespace

SharedSolverWrapper make_lbfgsb_driver(std::string_view direction,
                                       Options &opts) {
    static constexpr bool valid_config =
        std::is_same_v<LoadedProblem::config_t, InnerLBFGSBSolver::config_t>;
    if constexpr (valid_config)
        return make_lbfgsb_driver_impl<LoadedProblem>(direction, opts);
    else
        throw std::invalid_argument(
            "L-BFGS-B solver only supports double precision");
}
} // namespace alpaqa::driver

template class alpaqa::ALMSolver<alpaqa::driver::InnerLBFGSBSolver>;

#else

#include <alpaqa/driver/solver-driver.hpp>

namespace alpaqa::driver {
SharedSolverWrapper make_lbfgsb_driver(std::string_view, alpaqa::Options &) {
    throw std::invalid_argument(
        "This version of alpaqa was compiled without L-BFGS-B support.");
}
} // namespace alpaqa::driver

#endif
