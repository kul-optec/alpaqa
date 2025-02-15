#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include <alpaqa/config/config.hpp>
#include <alpaqa/params/options.hpp>
#include <alpaqa/problem-loader/problem-loader.hpp>
#include <alpaqa/problem/kkt-error.hpp>
#include <alpaqa/util/print.hpp>
#include <alpaqa/util/span.hpp>
#include <guanaqo/demangled-typename.hpp>
#include <guanaqo/string-util.hpp>
#include <alpaqa-version.h>

#include <alpaqa/driver/alpaqa-driver.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

#include "param-complete.hpp"

namespace alpaqa::driver {

USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);

int main(int argc, const char *argv[]) {
    // Check command line options
    if (argc < 1)
        return -1;
    if (argc == 1)
        return print_usage(argv[0], std::cout), 0;
    if (argc < 2)
        return print_usage(argv[0], std::cerr), -1;
    if (argv[1] == "-h"sv || argv[1] == "--help"sv || argv[1] == "?"sv)
        return print_usage(argv[0], std::cout), 0;
    if (argv[1] == "-v"sv || argv[1] == "--version"sv)
        return print_version(std::cout), 0;
    if (argv[1] == "--complete"sv) {
        if (argc < 4)
            return -1;
        print_completion(argv[2], argv[3]);
        return 0;
    }

    std::span args{argv, static_cast<size_t>(argc)};
    alpaqa::Options opts{argc - 2, argv + 2};

    // Check where to write the output to
    std::ofstream out_fstream;
    std::ostream &os = get_output_stream(opts, out_fstream, std::cout);

    // Check which problem to load
    auto [prob_path, prob_type] = get_problem_path(argv);

    // Check which solver to use
    SolverBuilders builders;
    auto [solver_builder, direction] = builders.get_solver_builder(opts);

    // Check output paths
    fs::path sol_output_dir = get_output_paths(opts);

    // Build solver
    auto solver = solver_builder(direction, opts);

    // Load problem
    os << "Loading " << prob_path << " ..." << std::endl;
    auto problem = alpaqa::load_problem(prob_type, prob_path, opts);
    // Print problem information
    bool show_funcs = false;
    set_params(show_funcs, "show_funcs", opts);
    print_problem_description(os, problem, show_funcs);
    os << std::endl;

    // Check options
    auto used       = opts.used();
    auto unused_opt = std::ranges::find(used, 0);
    auto unused_idx = static_cast<size_t>(unused_opt - used.begin());
    if (unused_opt != used.end())
        throw std::invalid_argument("Unused option: " +
                                    std::string(opts.options()[unused_idx]));

    // Solve
    auto solver_results = solver->run(problem, os);

    // Compute more statistics
    real_t f     = problem.problem.eval_objective(solver_results.solution);
    auto kkt_err = alpaqa::compute_kkt_error(
        problem.problem, solver_results.solution, solver_results.multipliers);
    BenchmarkResults results{
        .problem          = problem,
        .solver_results   = solver_results,
        .objective        = f + solver_results.h,
        .smooth_objective = f,
        .error            = kkt_err,
        .options          = opts.options(),
        .timestamp        = timestamp_ms<std::chrono::system_clock>().count(),
    };

    // Print results
    print_results(os, results);

    // Store solution
    if (!sol_output_dir.empty())
        store_solution(sol_output_dir, os, results, *solver, opts, args);
    return 0;
}

} // namespace alpaqa::driver

int main(int argc, const char *argv[]) try {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    return alpaqa::driver::main(argc, argv);
} catch (std::exception &e) {
    std::cerr << "Error: " << guanaqo::demangled_typename(typeid(e)) << ":\n  "
              << e.what() << std::endl;
    return -1;
}
