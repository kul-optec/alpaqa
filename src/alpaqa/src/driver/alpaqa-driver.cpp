#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/kkt-error.hpp>
#include <alpaqa/util/demangled-typename.hpp>
#include <alpaqa/util/print.hpp>
#include <alpaqa/util/string-util.hpp>
#include <alpaqa-version.h>

#include "fista-driver.hpp"
#include "ipopt-driver.hpp"
#include "lbfgsb-driver.hpp"
#include "options.hpp"
#include "panoc-driver.hpp"
#include "pantr-driver.hpp"
#include "param-complete.hpp"
#include "qpalm-driver.hpp"
#include "results.hpp"
#include "solver-driver.hpp"

#ifdef ALPAQA_WITH_EXTERNAL_CASADI
#include <casadi/config.h>
#endif
#ifdef ALPAQA_WITH_JSON
#include <nlohmann/json_fwd.hpp>
#endif
#ifdef ALPAQA_WITH_IPOPT
#include <IpoptConfig.h>
#endif

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
namespace fs = std::filesystem;
using namespace std::string_view_literals;

USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);

const auto *docs = R"==(
problem types:
    dl: Dynamically loaded problem using the DLProblem class.
        Specify the name of the registration function using the
        problem.register option, e.g. problem.register=register_alpaqa_problem.
        Further options can be passed to the problem using
        problem.<key>[=<value>].
    cs: Load a CasADi problem using the CasADiProblem class.
        If a .tsv file with the same name as the shared library file exists,
        the bounds and parameters will be loaded from that file. See
        CasADiProblem::load_numerical_data for more details.
        The problem parameter can be set using the problem.param option.
    cu: Load a CUTEst problem using the CUTEstProblem class.

methods:
    panoc[.<direction>]:
        PANOC solver with the given direction.
        Directions include: lbfgs, struclbfgs, anderson, convex-newton.
    zerofpr[.<direction>]:
        ZeroFPR solver, supports the same directions as PANOC.
    pantr:
        PANTR solver. Requires products with the Hessian of the augmented
        Lagrangian (unless dir.finite_diff=true).
    fista:
        FISTA (fast iterative shrinkage-thresholding algorithm). Only for
        convex problems.
    ipopt:
        Ipopt interior point solver. Requires Jacobian of the constraints
        and Hessian of the Lagrangian (unless finite memory is enabled).
    qpalm:
        QPALM proximal ALM QP solver. Assumes that the problem is a QP.
        Requires Jacobian of the constraints and Hessian of the Lagrangian.

options:
    Solver-specific options can be specified as key-value pairs, where the
    keys use periods to access struct members. For example,
    solver.Lipschitz.L_0=1e3.

    solver:  Parameters for the main (inner) solver.
    alm:     Parameters for the outer ALM solver (if applicable).
    dir:     Parameters for solver's direction provider (if applicable).
    accel:   Parameters for direction's accelerator (if applicable).
    out:     File to write output to (default: -, i.e. standard output).
    sol:     Folder to write the solutions (and optional statistics) to.
    x0:      Initial guess for the solution.
    mul_g0:  Initial guess for the multipliers of the general constraints.
    mul_x0:  Initial guess for the multipliers of the bound constraints on x.
    num_exp: Repeat the experiment this many times for more accurate timings.
    extra_stats: Log more per-iteration solver statistics, such as step sizes,
                 Newton step acceptance, and residuals. Requires `sol' to be set.
    show_funcs: Print an overview of the functions provided by the problem.

    The prefix @ can be added to the values of x0, mul_g0 and mul_x0 to read
    the values from the given CSV file.

    Options can be loaded from a JSON file by using an @ prefix. For example,
    an argument @options.json loads the options from a file options.json in the
    current directory. Multiple JSON files are processed in the order they
    appear in the command line arguments. Options specified on the command line
    always have precedence over options in a JSON file, regardless of order.

examples:
    alpaqa-driver problem.so \
        problem.register=register_alpaqa_problem \
        problem.custom_arg=foo \
        method=panoc.struclbfgs \
        accel.memory=50 \
        alm.{tolerance,dual_tolerance}=1e-8 \
        solver.print_interval=50 \
        x0=@/some/file.csv

    alpaqa-driver cs:build/casadi_problem.so \
        @options/default.json \
        problem.param=1,2,3 \
        method=ipopt \
        solver.tol=1e-8 solver.constr_viol_tol=1e-8 \
        solver.warm_start_init_point=yes \
        x0=@/some/file.csv \
        mul_g0=@/some/other/file.csv \
        mul_x0=@/yet/another/file.csv
)==";

void print_usage(const char *a0) {
    const auto *opts = " [<problem-type>:][<path>/]<name> [method=<solver>] "
                       "[<key>=<value>...]\n";
    std::cout << "alpaqa-driver " ALPAQA_VERSION_FULL " (" << alpaqa_build_time
              << ")\n\n"
                 "    Command-line interface to the alpaqa solvers.\n"
                 "    alpaqa is published under the LGPL-3.0.\n"
                 "    https://github.com/kul-optec/alpaqa"
                 "\n\n"
                 "    Usage: "
              << a0 << opts << docs << std::endl;
    std::cout << "Third-party libraries:\n"
              << "  * Eigen " << EIGEN_WORLD_VERSION << '.'
              << EIGEN_MAJOR_VERSION << '.' << EIGEN_MINOR_VERSION
              << " (https://gitlab.com/libeigen/eigen) - MPL-2.0\n"
#ifdef ALPAQA_WITH_EXTERNAL_CASADI
              << "  * CasADi " CASADI_VERSION_STRING
                 " (https://github.com/casadi/casadi) - LGPL-3.0-or-later\n"
#endif
#ifdef ALPAQA_WITH_CUTEST
              << "  * CUTEst"
                 " (https://github.com/ralna/CUTEst) - BSD-3-Clause\n"
#endif
#ifdef ALPAQA_WITH_LBFGSB
              << "  * L-BFGS-B 3.0 "
                 "(http://users.iems.northwestern.edu/~nocedal/lbfgsb.html) - "
                 "BSD-3-Clause\n"
#endif
#ifdef ALPAQA_WITH_IPOPT
              << "  * Ipopt " IPOPT_VERSION
                 " (https://github.com/coin-or/Ipopt) - EPL-2.0\n"
              << "  * MUMPS (https://mumps-solver.org) - CECILL-C\n"
              << "  * OpenBLAS (https://github.com/OpenMathLib/OpenBLAS) - "
                 "BSD-3-Clause\n"
#endif
#ifdef ALPAQA_WITH_QPALM
              << "  * QPALM " QPALM_VERSION_STR
                 " (https://github.com/kul-optec/QPALM) - LGPL-3.0\n"
#endif
#ifdef ALPAQA_WITH_JSON
              << "  * nlohmann/json " << NLOHMANN_JSON_VERSION_MAJOR << '.'
              << NLOHMANN_JSON_VERSION_MINOR << '.'
              << NLOHMANN_JSON_VERSION_PATCH
              << " (https://github.com/nlohmann/json) - MIT\n"
#endif
              << std::endl;
}

void print_version() {
    std::cout << ALPAQA_VERSION_FULL " (" << ALPAQA_BUILD_TIME << ")\n";
}

/// Split the string @p s on the first occurrence of @p tok.
/// Returns ("", s) if tok was not found.
auto split_once(std::string_view s, char tok = '.') {
    auto tok_pos = s.find(tok);
    if (tok_pos == s.npos)
        return std::make_tuple(std::string_view{}, s);
    std::string_view key{s.begin(), s.begin() + tok_pos};
    std::string_view rem{s.begin() + tok_pos + 1, s.end()};
    return std::make_tuple(key, rem);
}

std::ostream &get_output_stream(Options &opts, std::ofstream &out_fstream) {
    std::string out_path = "-";
    set_params(out_path, "out", opts);
    if (out_path != "-")
        if (out_fstream.open(out_path); !out_fstream)
            throw std::runtime_error("Unable to open '" + out_path + "'");
    return out_fstream.is_open() ? out_fstream : std::cout;
}

std::string get_output_paths(Options &opts) {
    std::string sol_path;
    set_params(sol_path, "sol", opts);
    return sol_path;
}

auto get_problem_path(const char *const *argv) {
    bool rel_to_exe              = argv[1][0] == '^';
    std::string_view prob_path_s = argv[1] + static_cast<ptrdiff_t>(rel_to_exe);
    std::string_view prob_type;
    std::tie(prob_type, prob_path_s) = split_once(prob_path_s, ':');
    fs::path prob_path{prob_path_s};
    if (rel_to_exe)
        prob_path = fs::canonical(fs::path(argv[0])).parent_path() / prob_path;
    return std::make_tuple(std::move(prob_path), prob_type);
}

void print_problem_description(std::ostream &os, LoadedProblem &problem,
                               bool show_funcs) {
    os << "Loaded problem \"" << problem.name << "\"\n"
       << "Number of variables:   " << problem.problem.get_n() << "\n"
       << "Number of constraints: " << problem.problem.get_m() << "\n";
    if (problem.nnz_jac_g)
        os << "Nonzeros in Jg:  " << *problem.nnz_jac_g << "\n";
    if (problem.nnz_hess_L)
        os << "Nonzeros in ∇²L: " << *problem.nnz_hess_L << "\n";
    if (problem.nnz_hess_ψ)
        os << "Nonzeros in ∇²ψ: " << *problem.nnz_hess_ψ << "\n";
    if (problem.box_constr_count)
        os << "Box constraints:"
           << "\n  Fixed variables:    " << problem.box_constr_count->eq
           << "\n  Bilateral:          " << problem.box_constr_count->lbub
           << "\n  Lower bound only:   " << problem.box_constr_count->lb
           << "\n  Upper bound only:   " << problem.box_constr_count->ub
           << "\n";
    if (problem.general_constr_count)
        os << "General constraints:"
           << "\n  Equality:           " << problem.general_constr_count->eq
           << "\n  Bilateral:          " << problem.general_constr_count->lbub
           << "\n  Lower bound only:   " << problem.general_constr_count->lb
           << "\n  Upper bound only:   " << problem.general_constr_count->ub
           << "\n";
    if (show_funcs) {
        os << "Provided functions:\n";
        alpaqa::print_provided_functions(os, problem.problem);
    }
}

auto get_solver_builder(Options &opts) {
    std::string method = "panoc", direction;
    set_params(method, "method", opts);
    std::tie(method, direction) = alpaqa::params::split_key(method, '.');
    // Dictionary of available solver builders
    std::map<std::string_view, solver_builder_func> solvers{
        {"panoc", make_panoc_driver}, {"zerofpr", make_zerofpr_driver},
        {"pantr", make_pantr_driver}, {"lbfgsb", make_lbfgsb_driver},
        {"fista", make_fista_driver}, {"ipopt", make_ipopt_driver},
        {"qpalm", make_qpalm_driver},
    };
    // Find the selected solver builder
    auto solver_it = solvers.find(method);
    if (solver_it == solvers.end())
        throw std::invalid_argument(
            "Unknown solver '" + std::string(method) + "'\n" +
            "  Available solvers: " +
            alpaqa::util::join(std::views::keys(solvers)));
    ;
    return std::make_tuple(std::move(solver_it->second), direction);
}

void store_solution(const fs::path &sol_output_dir, std::ostream &os,
                    BenchmarkResults &results, auto &solver,
                    [[maybe_unused]] const Options &opts,
                    std::span<const char *> argv) {
    const auto &sol_res = results.solver_results;
    auto timestamp_str  = std::to_string(results.timestamp);
    auto rnd_str        = random_hex_string(std::random_device());
    auto name           = results.problem.path.stem().string();
    if (name == "PROBLEM")
        name = results.problem.name;
    auto suffix = '_' + name + '_' + timestamp_str + '_' + rnd_str;
    fs::create_directories(sol_output_dir);
    std::array solutions{
        std::tuple{"solution", "sol_x", &sol_res.solution},
        std::tuple{"multipliers for g", "mul_g", &sol_res.multipliers},
        std::tuple{"multipliers for x", "mul_x", &sol_res.multipliers_bounds},
    };
    for (auto [name, fname, value] : solutions) {
        if (value->size() == 0)
            continue;
        auto pth = sol_output_dir / (std::string(fname) + suffix + ".csv");
        os << "Writing " << name << " to " << pth << std::endl;
        std::ofstream output_file(pth);
        alpaqa::print_csv(output_file, *value);
    }
    {
        auto pth = sol_output_dir / ("cmdline" + suffix + ".txt");
        os << "Writing arguments to " << pth << std::endl;
        std::ofstream output_file(pth);
        for (const char *arg : argv)
            output_file << std::quoted(arg, '\'') << ' ';
        output_file << '\n';
    }
    if (solver->has_statistics()) {
        auto pth = sol_output_dir / ("stats" + suffix + ".csv");
        os << "Writing statistics to " << pth << std::endl;
        std::ofstream output_file(pth);
        solver->write_statistics_to_stream(output_file);
    }
#if ALPAQA_WITH_JSON
    {
        auto pth = sol_output_dir / ("options" + suffix + ".json");
        os << "Writing options to " << pth << std::endl;
        std::ofstream output_file(pth);
        output_file << opts.get_json_out() << '\n';
    }
#endif
}

int main(int argc, const char *argv[]) try {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    // Check command line options
    if (argc < 1)
        return -1;
    if (argc == 1)
        return print_usage(argv[0]), 0;
    if (argc < 2)
        return print_usage(argv[0]), -1;
    if (argv[1] == "-h"sv || argv[1] == "--help"sv || argv[1] == "?"sv)
        return print_usage(argv[0]), 0;
    if (argv[1] == "-v"sv || argv[1] == "--version"sv)
        return print_version(), 0;
    if (argv[1] == "--complete"sv) {
        if (argc < 4)
            return -1;
        print_completion(argv[2], argv[3]);
        return 0;
    }

    std::span args{argv, static_cast<size_t>(argc)};
    Options opts{argc - 2, argv + 2};

    // Check where to write the output to
    std::ofstream out_fstream;
    std::ostream &os = get_output_stream(opts, out_fstream);

    // Check which problem to load
    auto [prob_path, prob_type] = get_problem_path(argv);

    // Check which solver to use
    auto [solver_builder, direction] = get_solver_builder(opts);

    // Check output paths
    fs::path sol_output_dir = get_output_paths(opts);

    // Build solver
    auto solver = solver_builder(direction, opts);

    // Load problem
    os << "Loading " << prob_path << " ..." << std::endl;
    auto problem = load_problem(prob_type, prob_path.parent_path(),
                                prob_path.filename(), opts);
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
    real_t f     = problem.problem.eval_f(solver_results.solution);
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
        store_solution(sol_output_dir, os, results, solver, opts, args);

} catch (std::exception &e) {
    std::cerr << "Error: " << demangled_typename(typeid(e)) << ":\n  "
              << e.what() << std::endl;
    return -1;
}
