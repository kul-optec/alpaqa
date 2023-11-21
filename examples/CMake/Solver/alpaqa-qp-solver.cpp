#include <alpaqa/panoc-alm.hpp>
#include <alpaqa/problem/problem-with-counters.hpp>
#include <alpaqa/util/demangled-typename.hpp>

#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace fs = std::filesystem;

#include "problem.hpp"

int main(int argc, const char *argv[]) try {
    // Configure the data types to use
    USING_ALPAQA_CONFIG(Problem::config_t);

    // Parse command line arguments
    if (argc < 2)
        throw std::invalid_argument("Missing command-line argument 'path'");
    fs::path problem_dir(argv[1]);

    // Instantiate the problem
    Problem problem{problem_dir};

    // Wrap the problem to count the number of function evaluations
    auto counted_problem = alpaqa::problem_with_counters_ref(problem);

    // Define the solvers to use
    using Direction   = alpaqa::LBFGSDirection<config_t>;
    using InnerSolver = alpaqa::PANOCSolver<Direction>;
    using OuterSolver = alpaqa::ALMSolver<InnerSolver>;

    // Settings for the outer augmented Lagrangian method
    OuterSolver::Params almparam{
        .tolerance             = 1e-8, // stationarity tolerance
        .dual_tolerance        = 1e-8, // constraint violation tolerance
        .penalty_update_factor = 10,   // penalty update factor
        .max_iter              = 20,
        .print_interval        = 1,
    };
    // Settings for the inner PANOC solver
    InnerSolver::Params panocparam{
        .max_iter       = 500,
        .print_interval = 1,
    };
    // Settings for the L-BFGS algorithm used by PANOC
    Direction::AcceleratorParams lbfgsparam{
        .memory = 5,
    };
    // Create an ALM solver using PANOC as inner solver
    OuterSolver solver{
        almparam,                 // params for outer solver
        {panocparam, lbfgsparam}, // inner solver
    };

    // Initial guess
    vec x(2), y(1);
    x << 2, 2; // decision variables
    y << 1;    // Lagrange multipliers

    // Solve the problem
    auto stats = solver(counted_problem, x, y);
    // y and x have been overwritten by the solution

    // Print the results
    std::cout << '\n'
              << *counted_problem.evaluations << '\n'
              << "status: " << stats.status << '\n'
              << "f = " << problem.eval_f(x) << '\n'
              << "inner iterations: " << stats.inner.iterations << '\n'
              << "outer iterations: " << stats.outer_iterations << '\n'
              << "ε = " << stats.ε << '\n'
              << "δ = " << stats.δ << '\n'
              << "elapsed time:     "
              << std::chrono::duration<double>{stats.elapsed_time}.count()
              << " s" << '\n'
              << "x = " << x.transpose() << '\n'
              << "y = " << y.transpose() << '\n'
              << "avg τ = " << (stats.inner.sum_τ / stats.inner.count_τ) << '\n'
              << "L-BFGS rejected = " << stats.inner.lbfgs_rejected << '\n'
              << "L-BFGS failures = " << stats.inner.lbfgs_failures << '\n'
              << "Line search failures = " << stats.inner.linesearch_failures
              << '\n'
              << std::endl;
    return stats.status == alpaqa::SolverStatus::Converged ? 0 : 1;
} catch (const std::exception &e) {
    std::cerr << demangled_typename(typeid(e)) << ": " << e.what() << '\n';
    return 255;
}
