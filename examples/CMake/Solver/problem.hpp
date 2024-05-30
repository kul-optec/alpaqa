#pragma once

#include <alpaqa/problem/box-constr-problem.hpp>

#include <filesystem>

/// Problem formulation
///
///     minimize  ½ xᵀQx + cᵀx
///      s.t.     Ax ≤ b
struct Problem : alpaqa::BoxConstrProblem<alpaqa::DefaultConfig> {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);

    mat Q{num_variables, num_variables};   ///< Hessian matrix
    vec c{num_variables};                  ///< Gradient vextor
    mat A{num_constraints, num_variables}; ///< Constraint matrix
    mutable vec Qx{num_variables};         ///< Temporary work vector

    /// Constructor loads problem data from the given directory
    Problem(const std::filesystem::path &problem_dir);

    /// Evaluate the cost
    real_t eval_objective(crvec x) const;
    /// Evaluate the gradient of the cost
    void eval_objective_gradient(crvec x, rvec gr) const;
    /// Evaluate the constraints
    void eval_constraints(crvec x, rvec g) const;
    /// Evaluate a matrix-vector product with the gradient of the constraints
    void eval_constraints_gradient_product(crvec x, crvec y, rvec gr) const;
};
