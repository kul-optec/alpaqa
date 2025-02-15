#pragma once

#include <alpaqa/drivers-export.h>

#include <alpaqa/config/config.hpp>
#include <alpaqa/params/options.hpp>

#include <alpaqa/driver/results.hpp>
#include <alpaqa/driver/solver-driver.hpp>

#include <filesystem>
#include <iosfwd>
#include <map>
#include <span>
#include <string_view>
#include <tuple>

namespace alpaqa::driver {

namespace fs = std::filesystem;
using namespace std::string_view_literals;

ALPAQA_DRIVERS_EXPORT void print_usage(const char *a0, std::ostream &os);
ALPAQA_DRIVERS_EXPORT void print_version(std::ostream &os);

struct ALPAQA_DRIVERS_EXPORT SolverBuilders {
    SolverBuilders(std::string default_solver = "panoc");
    std::tuple<solver_builder_func, std::string>
    get_solver_builder(alpaqa::Options &opts) const;
    void register_solver_builder(std::string_view name,
                                 solver_builder_func builder);
    std::string default_solver;
    std::map<std::string_view, solver_builder_func> solvers;
};

ALPAQA_DRIVERS_EXPORT std::string
store_solution(const fs::path &sol_output_dir, std::ostream &os,
               BenchmarkResults &results, const SolverWrapper &solver,
               [[maybe_unused]] const alpaqa::Options &opts,
               std::span<const char *const> argv);

ALPAQA_DRIVERS_EXPORT std::ostream &
get_output_stream(alpaqa::Options &opts, std::ofstream &out_fstream,
                  std::ostream &default_stream);

ALPAQA_DRIVERS_EXPORT std::string get_output_paths(alpaqa::Options &opts);

ALPAQA_DRIVERS_EXPORT std::tuple<fs::path, std::string_view>
get_problem_path(const char *const *argv);

} // namespace alpaqa::driver
