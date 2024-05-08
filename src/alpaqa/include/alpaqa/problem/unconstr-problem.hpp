#pragma once

#include <alpaqa/config/config.hpp>

#include <numeric>

namespace alpaqa {

/// Implements common problem functions for minimization problems without
/// constraints. Meant to be used as a base class for custom problem
/// implementations.
/// @ingroup grp_Problems
template <Config Conf>
class UnconstrProblem {
  public:
    USING_ALPAQA_CONFIG(Conf);

    /// Number of decision variables, dimension of x
    length_t num_variables;

    /// @param num_variables Number of decision variables
    UnconstrProblem(length_t num_variables) : num_variables{num_variables} {}

    /// Change the number of decision variables.
    void resize(length_t num_variables) { this->num_variables = num_variables; }

    UnconstrProblem(const UnconstrProblem &)                = default;
    UnconstrProblem &operator=(const UnconstrProblem &)     = default;
    UnconstrProblem(UnconstrProblem &&) noexcept            = default;
    UnconstrProblem &operator=(UnconstrProblem &&) noexcept = default;

    /// Number of decision variables @f$ n @f$, @ref num_variables
    length_t get_num_variables() const { return num_variables; }
    /// Number of constraints (always zero)
    length_t get_num_constraints() const { return 0; }

    /// No-op, no constraints.
    /// @see @ref TypeErasedProblem::eval_constraints
    void eval_constraints(crvec, rvec) const {}
    /// Constraint gradient is always zero.
    /// @see @ref TypeErasedProblem::eval_constraints_gradient_product
    void eval_constraints_gradient_product(crvec, crvec, rvec grad) const { grad.setZero(); }
    /// Constraint Jacobian is always empty.
    /// @see @ref TypeErasedProblem::eval_constraints_jacobian
    void eval_constraints_jacobian(crvec, rvec) const {}
    /// Constraint gradient is always zero.
    /// @see @ref TypeErasedProblem::eval_grad_gi
    void eval_grad_gi(crvec, index_t, rvec grad_gi) const { grad_gi.setZero(); }

    /// No proximal mapping, just a forward (gradient) step.
    /// @see @ref TypeErasedProblem::eval_proximal_gradient_step
    real_t eval_proximal_gradient_step(real_t γ, crvec x, crvec grad_ψ, rvec x̂, rvec p) const {
        p = -γ * grad_ψ;
        x̂ = x + p;
        return 0;
    }

    /// @see @ref TypeErasedProblem::eval_projecting_difference_constraints
    void eval_projecting_difference_constraints(crvec, rvec) const {}

    /// @see @ref TypeErasedProblem::eval_projection_multipliers
    void eval_projection_multipliers(rvec, real_t) const {}

    /// @see @ref TypeErasedProblem::eval_inactive_indices_res_lna
    index_t eval_inactive_indices_res_lna(real_t, crvec, crvec, rindexvec J) const {
        std::iota(J.begin(), J.end(), index_t{0});
        return J.size();
    }

    /// @see @ref TypeErasedProblem::get_name
    [[nodiscard]] std::string get_name() const { return "UnconstrProblem"; }
};

} // namespace alpaqa
