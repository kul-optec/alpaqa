#include "problem.hpp"

#include <alpaqa/util/io/csv.hpp>
#include <fstream>
#include <tuple>

namespace {

auto read_dimensions(const std::filesystem::path &problem_dir) {
    USING_ALPAQA_CONFIG(Problem::config_t);
    Eigen::Vector<index_t, 2> dimensions;
    std::ifstream file{problem_dir / "dim.csv"};
    alpaqa::csv::read_row(file, rindexvec{dimensions});
    return std::make_tuple(dimensions(0), dimensions(1));
}

} // namespace

Problem::Problem(const std::filesystem::path &problem_dir)
    : alpaqa::BoxConstrProblem<config_t>{read_dimensions(problem_dir)} {
    // Initialize problem matrices
    {
        std::ifstream file{problem_dir / "Q.csv"};
        for (index_t r = 0; r < n; ++r)
            alpaqa::csv::read_row(file, rvec{Q.col(r)}); // Assume symmetric
    }
    {
        std::ifstream file{problem_dir / "c.csv"};
        alpaqa::csv::read_row(file, rvec{c});
    }
    {
        std::ifstream file{problem_dir / "A.csv"};
        A.resize(n, m); // Reading the transpose of A is easier
        for (index_t r = 0; r < m; ++r)
            alpaqa::csv::read_row(file, rvec{A.col(r)});
        A.transposeInPlace();
    }
    {
        std::ifstream file{problem_dir / "b.csv"};
        alpaqa::csv::read_row(file, rvec{D.upperbound});
    }
    {
        std::ifstream file{problem_dir / "lbx.csv"};
        alpaqa::csv::read_row(file, rvec{C.lowerbound});
    }
    {
        std::ifstream file{problem_dir / "ubx.csv"};
        alpaqa::csv::read_row(file, rvec{C.upperbound});
    }
}

auto Problem::eval_f(crvec x) const -> real_t {
    Qx.noalias() = Q * x;
    return 0.5 * x.dot(Qx) + c.dot(x);
}
void Problem::eval_grad_f(crvec x, rvec gr) const { gr.noalias() = Q * x + c; }
void Problem::eval_g(crvec x, rvec g) const { g.noalias() = A * x; }
void Problem::eval_grad_g_prod(crvec x, crvec y, rvec gr) const {
    (void)x;
    gr.noalias() = A.transpose() * y;
}
