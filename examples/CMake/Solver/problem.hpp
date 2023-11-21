#pragma once

#include <alpaqa/problem/box-constr-problem.hpp>

#include <filesystem>

/// Problem formulation
///
///     minimize  ½ xᵀQx + qᵀx
///      s.t.     Ax ≤ b
struct Problem : alpaqa::BoxConstrProblem<alpaqa::DefaultConfig> {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);

    mat Q{n, n};       ///< Hessian matrix
    vec q{n};          ///< Gradient vextor
    mat A{m, n};       ///< Constraint matrix
    mutable vec Qx{n}; ///< Temporary work vector

    /// Constructor loads problem data from the given directory
    Problem(const std::filesystem::path &problem_dir);

    /// Evaluate the cost
    real_t eval_f(crvec x) const;
    /// Evaluate the gradient of the cost
    void eval_grad_f(crvec x, rvec gr) const;
    /// Evaluate the constraints
    void eval_g(crvec x, rvec g) const;
    /// Evaluate a matrix-vector product with the gradient of the constraints
    void eval_grad_g_prod(crvec x, crvec y, rvec gr) const;
};
