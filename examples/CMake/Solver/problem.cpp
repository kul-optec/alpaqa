#include "problem.hpp"

#include <fstream>
#include <tuple>

#include <alpaqa/util/span.hpp>
#include <guanaqo/io/csv.hpp>
using namespace guanaqo::io;

namespace {

auto read_dimensions(const std::filesystem::path &problem_dir) {
    USING_ALPAQA_CONFIG(Problem::config_t);
    Eigen::Vector<index_t, 2> dimensions;
    std::ifstream file{problem_dir / "dim.csv"};
    csv_read_row(file, alpaqa::as_span(dimensions));
    return std::make_tuple(dimensions(0), dimensions(1));
}

} // namespace

Problem::Problem(const std::filesystem::path &problem_dir)
    : alpaqa::BoxConstrProblem<config_t>{read_dimensions(problem_dir)} {
    const auto n = num_variables, m = num_constraints;
    // Initialize problem matrices
    {
        std::ifstream file{problem_dir / "Q.csv"};
        for (index_t r = 0; r < n; ++r)
            csv_read_row(file, alpaqa::as_span(Q.col(r))); // Assume symmetric
    }
    {
        std::ifstream file{problem_dir / "c.csv"};
        csv_read_row(file, alpaqa::as_span(c));
    }
    {
        std::ifstream file{problem_dir / "A.csv"};
        A.resize(n, m); // Reading the transpose of A is easier
        for (index_t r = 0; r < m; ++r)
            csv_read_row(file, alpaqa::as_span(A.col(r)));
        A.transposeInPlace();
    }
    {
        std::ifstream file{problem_dir / "b.csv"};
        csv_read_row(file, alpaqa::as_span(general_bounds.upper));
    }
    {
        std::ifstream file{problem_dir / "lbx.csv"};
        csv_read_row(file, alpaqa::as_span(variable_bounds.lower));
    }
    {
        std::ifstream file{problem_dir / "ubx.csv"};
        csv_read_row(file, alpaqa::as_span(variable_bounds.upper));
    }
}

auto Problem::eval_objective(crvec x) const -> real_t {
    Qx.noalias() = Q * x;
    return 0.5 * x.dot(Qx) + c.dot(x);
}
void Problem::eval_objective_gradient(crvec x, rvec gr) const {
    gr.noalias() = Q * x + c;
}
void Problem::eval_constraints(crvec x, rvec g) const { g.noalias() = A * x; }
void Problem::eval_constraints_gradient_product(crvec x, crvec y,
                                                rvec gr) const {
    (void)x;
    gr.noalias() = A.transpose() * y;
}
