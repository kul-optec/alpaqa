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
    length_t n;

    /// @param n Number of decision variables
    UnconstrProblem(length_t n) : n{n} {}

    /// Change the number of decision variables.
    void resize(length_t n) { this->n = n; }

    UnconstrProblem(const UnconstrProblem &)                = default;
    UnconstrProblem &operator=(const UnconstrProblem &)     = default;
    UnconstrProblem(UnconstrProblem &&) noexcept            = default;
    UnconstrProblem &operator=(UnconstrProblem &&) noexcept = default;

    /// Number of decision variables, @ref n
    length_t get_n() const { return n; }
    /// Number of constraints (always zero)
    length_t get_m() const { return 0; }

    /// No-op, no constraints.
    /// @see @ref TypeErasedProblem::eval_g
    void eval_g(crvec, rvec) const {}
    /// Constraint gradient is always zero.
    /// @see @ref TypeErasedProblem::eval_grad_g_prod
    void eval_grad_g_prod(crvec, crvec, rvec grad) const { grad.setZero(); }
    /// Constraint Jacobian is always empty.
    /// @see @ref TypeErasedProblem::eval_jac_g
    void eval_jac_g(crvec, rvec) const {}
    /// Constraint gradient is always zero.
    /// @see @ref TypeErasedProblem::eval_grad_gi
    void eval_grad_gi(crvec, index_t, rvec grad_gi) const { grad_gi.setZero(); }

    /// No proximal mapping, just a forward (gradient) step.
    /// @see @ref TypeErasedProblem::eval_prox_grad_step
    real_t eval_prox_grad_step(real_t γ, crvec x, crvec grad_ψ, rvec x̂, rvec p) const {
        p = -γ * grad_ψ;
        x̂ = x + p;
        return 0;
    }

    /// @see @ref TypeErasedProblem::eval_proj_diff_g
    void eval_proj_diff_g(crvec, rvec) const {}

    /// @see @ref TypeErasedProblem::eval_proj_multipliers
    void eval_proj_multipliers(rvec, real_t) const {}

    /// @see @ref TypeErasedProblem::eval_inactive_indices_res_lna
    index_t eval_inactive_indices_res_lna(real_t, crvec, crvec, rindexvec J) const {
        std::iota(J.begin(), J.end(), index_t{0});
        return J.size();
    }
};

} // namespace alpaqa
