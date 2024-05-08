#pragma once

#include <alpaqa/problem/box.hpp>
#include <alpaqa/util/check-dim.hpp>

#include <utility>

namespace alpaqa {

/// Implements common problem functions for minimization problems with box
/// constraints. Meant to be used as a base class for custom problem
/// implementations.
/// Supports optional @f$ \ell_1 @f$-regularization.
/// @ingroup grp_Problems
template <Config Conf>
class BoxConstrProblem {
  public:
    USING_ALPAQA_CONFIG(Conf);
    using Box = alpaqa::Box<config_t>;

    /// Number of decision variables, dimension of x
    length_t num_variables;
    /// Number of constraints, dimension of g(x) and z
    length_t num_constraints;

    /// Create a problem with inactive boxes @f$ (-\infty, +\infty) @f$, with
    /// no @f$ \ell_1 @f$-regularization, and all general constraints handled
    /// using ALM.
    BoxConstrProblem(length_t num_variables,   ///< Number of decision variables
                     length_t num_constraints) ///< Number of constraints
        : num_variables{num_variables}, num_constraints{num_constraints} {}
    /// @copybrief BoxConstrProblem(length_t, length_t)
    /// @param dims Number of variables and number of constraints.
    BoxConstrProblem(std::tuple<length_t, length_t> dims)
        : BoxConstrProblem{get<0>(dims), get<1>(dims)} {}

    BoxConstrProblem(Box variable_bounds, Box general_bounds, vec l1_reg = vec(0),
                     index_t penalty_alm_split = 0)
        : num_variables{variable_bounds.lower.size()}, num_constraints{general_bounds.lower.size()},
          variable_bounds{std::move(variable_bounds)}, general_bounds{std::move(general_bounds)},
          l1_reg{std::move(l1_reg)}, penalty_alm_split{penalty_alm_split} {}

    /// Change the dimensions of the problem (number of decision variables and
    /// number of constraints).
    /// Destructive: resizes and/or resets the members @ref variable_bounds,
    /// @ref general_bounds, @ref l1_reg and @ref penalty_alm_split.
    void resize(length_t num_variables,   ///< Number of decision variables
                length_t num_constraints) ///< Number of constraints
    {
        if (std::exchange(this->num_variables, num_variables) != num_variables) {
            variable_bounds = Box{num_variables};
            if (l1_reg.size() > 1)
                l1_reg.resize(0);
        }
        if (std::exchange(this->num_constraints, num_constraints) != num_constraints) {
            general_bounds    = Box{num_constraints};
            penalty_alm_split = 0;
        }
    }

    BoxConstrProblem(const BoxConstrProblem &)                = default;
    BoxConstrProblem &operator=(const BoxConstrProblem &)     = default;
    BoxConstrProblem(BoxConstrProblem &&) noexcept            = default;
    BoxConstrProblem &operator=(BoxConstrProblem &&) noexcept = default;

    /// Constraints of the decision variables, @f$ x \in C @f$
    Box variable_bounds{this->num_variables};
    /// Other constraints, @f$ g(x) \in D @f$
    Box general_bounds{this->num_constraints};
    /// @f$ \ell_1 @f$ (1-norm) regularization parameter.
    /// Possible dimensions are: @f$ 0 @f$ (no regularization), @f$ 1 @f$ (a
    /// single scalar factor), or @f$ n @f$ (a different factor for each
    /// variable).
    vec l1_reg{};

    /// Components of the constraint function with indices below this number are
    /// handled using a quadratic penalty method rather than using an
    /// augmented Lagrangian method. Specifically, the Lagrange multipliers for
    /// these components (which determine the shifts in ALM) are kept at zero.
    index_t penalty_alm_split = 0;

    /// Number of decision variables @f$ n @f$, @ref num_variables
    length_t get_num_variables() const { return num_variables; }
    /// Number of constraints @f$ m @f$, @ref num_constraints
    length_t get_num_constraints() const { return num_constraints; }

    /** Projected gradient step for rectangular box C.
      * @f[ \begin{aligned} \hat x &= \Pi_C(x - \gamma\nabla\psi(x)) \\
      * p &= \hat x - x \\
      * &= \max(\underline x - x, \;\min(-\gamma\nabla\psi(x), \overline x - x)
      * \end{aligned} @f] */
    static real_t eval_proj_grad_step_box(const Box &C, real_t γ, crvec x, crvec grad_ψ, rvec x̂,
                                          rvec p) {
        p = (-γ * grad_ψ).cwiseMax(C.lower - x).cwiseMin(C.upper - x);
        x̂ = x + p;
        return real_t(0);
    }

    /** Proximal gradient step for rectangular box C with ℓ₁-regularization.
      * @f[ \begin{aligned} h(x) &= \|x\|_1 + \delta_C(x) \\
      * \hat x &= \prox_{\gamma h}(x - \gamma\nabla\psi(x)) \\
      * &= -\max\big(
      *         x - \overline x,
      *         \;\min\big(
      *             x - \underline x,
      *             \;\min\big(
      *                 \gamma(\nabla\psi(x) + \lambda),
      *                 \;\max\big(
      *                     \gamma(\nabla\psi(x) - \lambda),
      *                     x
      *                 \big)
      *             \big)
      *         \big)
      *     \big) \end{aligned} @f] */
    static void eval_prox_grad_step_box_l1_impl(const Box &C, const auto &λ, real_t γ, crvec x,
                                                crvec grad_ψ, rvec x̂, rvec p) {
        p = -x.cwiseMax(γ * (grad_ψ - λ))
                 .cwiseMin(γ * (grad_ψ + λ))
                 .cwiseMin(x - C.lower)
                 .cwiseMax(x - C.upper);
        x̂ = x + p;
    }
    /// @copydoc eval_prox_grad_step_box_l1_impl
    static real_t eval_prox_grad_step_box_l1(const Box &C, const auto &λ, real_t γ, crvec x,
                                             crvec grad_ψ, rvec x̂, rvec p) {
        eval_prox_grad_step_box_l1_impl(C, λ, γ, x, grad_ψ, x̂, p);
        using vec_util::norm_1;
        return norm_1(x̂.cwiseProduct(λ));
    }

    /// @copydoc eval_prox_grad_step_box_l1_impl
    static real_t eval_prox_grad_step_box_l1_scal(const Box &C, real_t λ, real_t γ, crvec x,
                                                  crvec grad_ψ, rvec x̂, rvec p) {
        auto n     = x.size();
        auto λ_vec = vec::Constant(n, λ);
        eval_prox_grad_step_box_l1_impl(C, λ_vec, γ, x, grad_ψ, x̂, p);
        using vec_util::norm_1;
        return λ * norm_1(x̂);
    }

    /// @see @ref TypeErasedProblem::eval_proximal_gradient_step
    real_t eval_proximal_gradient_step(real_t γ, crvec x, crvec grad_ψ, rvec x̂, rvec p) const {
        if (l1_reg.size() == 0)
            return eval_proj_grad_step_box(variable_bounds, γ, x, grad_ψ, x̂, p);
        else if constexpr (requires { l1_reg(0); })
            if (l1_reg.size() == 1)
                return eval_prox_grad_step_box_l1_scal(variable_bounds, l1_reg(0), γ, x, grad_ψ, x̂,
                                                       p);
        return eval_prox_grad_step_box_l1(variable_bounds, l1_reg, γ, x, grad_ψ, x̂, p);
    }

    /// @see @ref TypeErasedProblem::eval_projecting_difference_constraints
    void eval_projecting_difference_constraints(crvec z, rvec p) const {
        p = projecting_difference(z, general_bounds);
    }

    static void eval_proj_multipliers_box(const Box &D, rvec y, real_t M,
                                          index_t penalty_alm_split) {
        auto num_alm  = y.size() - penalty_alm_split;
        auto y_qpm    = y.topRows(penalty_alm_split);
        auto y_alm    = y.bottomRows(num_alm);
        auto z_alm_lb = D.lower.bottomRows(num_alm);
        auto z_alm_ub = D.upper.bottomRows(num_alm);
        y_qpm.setZero();
        // If there's no lower bound, the multipliers can only be positive
        auto y_alm_lb = (z_alm_lb.array() == -alpaqa::inf<config_t>).select(vec::Zero(num_alm), -M);
        // If there's no upper bound, the multipliers can only be negative
        auto y_alm_ub = (z_alm_ub.array() == +alpaqa::inf<config_t>).select(vec::Zero(num_alm), +M);
        y_alm         = y_alm.cwiseMax(y_alm_lb).cwiseMin(y_alm_ub);
    }

    /// @see @ref TypeErasedProblem::eval_projection_multipliers
    void eval_projection_multipliers(rvec y, real_t M) const {
        eval_proj_multipliers_box(general_bounds, y, M, penalty_alm_split);
    }

    /// @see @ref TypeErasedProblem::get_variable_bounds
    const Box &get_variable_bounds() const { return variable_bounds; }
    /// @see @ref TypeErasedProblem::get_general_bounds
    const Box &get_general_bounds() const { return general_bounds; }

    /// Only supported if the ℓ₁-regularization term is zero.
    /// @see @ref TypeErasedProblem::provides_get_variable_bounds
    [[nodiscard]] bool provides_get_variable_bounds() const {
        const auto nλ = l1_reg.size();
        if (nλ == 0)
            return true;
        if constexpr (requires { l1_reg(0); })
            return (nλ == 1 && l1_reg(0) == 0);
        return false;
    }

    /// @see @ref TypeErasedProblem::eval_inactive_indices_res_lna
    index_t eval_inactive_indices_res_lna(real_t γ, crvec x, crvec grad_ψ, rindexvec J) const
        requires config_t::supports_indexvec
    {
        index_t nJ = 0;
        // Helper that adds i to index set J if x ∊ C
        const auto add_to_J_if_in_box_interior = [&](real_t x_fw, index_t i) {
            if (variable_bounds.lower(i) < x_fw && x_fw < variable_bounds.upper(i))
                J(nJ++) = i;
        };
        // Update the index set J for the general box + l1 case
        const auto update_J_general = [&](real_t λ, real_t x_fw, index_t i) {
            if (λ == 0) {
                add_to_J_if_in_box_interior(x_fw, i);
            } else {
                if (x_fw > γ * λ)
                    add_to_J_if_in_box_interior(x_fw - γ * λ, i);
                else if (x_fw < -γ * λ)
                    add_to_J_if_in_box_interior(x_fw + γ * λ, i);
            }
        };
        const auto nλ     = l1_reg.size();
        const bool λ_is_0 = nλ == 0 || (nλ == 1 && l1_reg(0) == 0);
        // Only box constraints
        if (λ_is_0)
            for (index_t i = 0; i < num_variables; ++i) {
                real_t x_fw = x(i) - γ * grad_ψ(i);
                add_to_J_if_in_box_interior(x_fw, i);
            }
        // Box constraints and l1
        else
            for (index_t i = 0; i < num_variables; ++i) {
                real_t λi   = nλ == 0 ? 0 : nλ == 1 ? l1_reg(0) : l1_reg(i);
                real_t x_fw = x(i) - γ * grad_ψ(i);
                update_J_general(λi, x_fw, i);
            }
        return nJ;
    }

    /// @see @ref TypeErasedProblem::check
    void check() const {
        util::check_dim_msg(variable_bounds.lower, num_variables,
                            "Length of problem.variable_bounds.lower does not match "
                            "problem size problem.num_variables");
        util::check_dim_msg(variable_bounds.upper, num_variables,
                            "Length of problem.variable_bounds.upper does not match "
                            "problem size problem.num_variables");
        util::check_dim_msg(general_bounds.lower, num_constraints,
                            "Length of problem.general_bounds.lower does not match "
                            "problem size problem.num_constraints");
        util::check_dim_msg(general_bounds.upper, num_constraints,
                            "Length of problem.general_bounds.upper does not match "
                            "problem size problem.num_constraints");
        if (l1_reg.size() > 1)
            util::check_dim_msg(l1_reg, num_variables,
                                "Length of problem.l1_reg does not match "
                                "problem size problem.num_variables, 1 or 0");
        if (penalty_alm_split < 0 || penalty_alm_split > num_constraints)
            throw std::invalid_argument("Invalid penalty_alm_split");
    }

    /// @see @ref TypeErasedProblem::get_name
    [[nodiscard]] std::string get_name() const { return "BoxConstrProblem"; }
};

} // namespace alpaqa
