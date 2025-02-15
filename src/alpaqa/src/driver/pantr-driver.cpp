#include <alpaqa/inner/directions/pantr/newton-tr.hpp>
#include <alpaqa/inner/pantr.hpp>
#include <alpaqa/params/options.hpp>
#include <guanaqo/string-util.hpp>

#include <alpaqa/driver/alm-driver.hpp>
#include <alpaqa/driver/cancel.hpp>
#include <alpaqa/driver/solver-driver.hpp>
#include "extra-stats.hpp"
#include "pantr-driver.hpp"

namespace alpaqa::driver {
namespace {

template <class T>
struct tag_t {};

template <template <class Direction> class Solver>
SharedSolverWrapper
make_pantr_like_solver(std::string_view direction,
                       [[maybe_unused]] alpaqa::Options &opts) {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    auto builder = []<class Direction>(tag_t<Direction>) {
        return
            [](std::string_view, alpaqa::Options &opts) -> SharedSolverWrapper {
                using collector_t = AlpaqaSolverStatsCollector<config_t>;
                std::shared_ptr<collector_t> collector;
                auto inner_solver = make_inner_solver<Solver<Direction>>(opts);
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
                            N_exp](alpaqa::LoadedProblem &problem,
                                   std::ostream &os) mutable -> SolverResults {
                    auto cancel = alpaqa::attach_cancellation(solver);
                    return run_alm_solver(problem, solver, os, N_exp);
                };
                return std::make_shared<AlpaqaSolverWrapperStats<config_t>>(
                    std::move(run), std::move(collector));
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
        throw std::invalid_argument("Unknown direction '" +
                                    std::string(direction) + "'\n" +
                                    "  Available directions: " +
                                    guanaqo::join(std::views::keys(builders)));
}

} // namespace

SharedSolverWrapper make_pantr_driver(std::string_view direction,
                                      alpaqa::Options &opts) {
    return make_pantr_like_solver<alpaqa::PANTRSolver>(direction, opts);
}

} // namespace alpaqa::driver
