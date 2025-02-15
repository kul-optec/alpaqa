#include <alpaqa/implementation/outer/alm.tpp>
#include <alpaqa/inner/fista.hpp>
#include <alpaqa/params/options.hpp>

#include <alpaqa/driver/alm-driver.hpp>
#include <alpaqa/driver/cancel.hpp>
#include <alpaqa/driver/solver-driver.hpp>
#include "fista-driver.hpp"

namespace alpaqa::driver {
namespace {

using FISTASolver = alpaqa::FISTASolver<alpaqa::DefaultConfig>;

auto make_inner_fista_solver(alpaqa::Options &opts) {
    USING_ALPAQA_CONFIG(FISTASolver::config_t);
    // Settings for the solver
    FISTASolver::Params solver_param;
    solver_param.max_iter       = 50'000;
    solver_param.print_interval = 0;
    solver_param.stop_crit      = alpaqa::PANOCStopCrit::FPRNorm;
    set_params(solver_param, "solver", opts);
    return FISTASolver{solver_param};
}

template <class LoadedProblem>
SharedSolverWrapper make_fista_driver_impl(std::string_view direction,
                                           alpaqa::Options &opts) {
    if (!direction.empty())
        throw std::invalid_argument(
            "FISTA solver does not support any directions");
    auto inner_solver = make_inner_fista_solver(opts);
    auto solver       = make_alm_solver(std::move(inner_solver), opts);
    unsigned N_exp    = 0;
    set_params(N_exp, "num_exp", opts);
    return std::make_shared<SolverWrapper>(
        [solver{std::move(solver)}, N_exp](
            LoadedProblem &problem, std::ostream &os) mutable -> SolverResults {
            auto cancel = alpaqa::attach_cancellation(solver);
            return run_alm_solver(problem, solver, os, N_exp);
        });
}

} // namespace

SharedSolverWrapper make_fista_driver(std::string_view direction,
                                      alpaqa::Options &opts) {
    return make_fista_driver_impl<alpaqa::LoadedProblem>(direction, opts);
}

} // namespace alpaqa::driver

template class alpaqa::ALMSolver<alpaqa::driver::FISTASolver>;
