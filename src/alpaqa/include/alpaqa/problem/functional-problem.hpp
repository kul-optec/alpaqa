#pragma once

#include <alpaqa/problem/box-constr-problem.hpp>
#include <alpaqa/util/alloc-check.hpp>

namespace alpaqa {

/// Problem class that allows specifying the basic functions as C++
/// `std::function`s.
/// @ingroup grp_Problems
template <Config Conf = DefaultConfig>
class FunctionalProblem : public BoxConstrProblem<Conf> {
  public:
    USING_ALPAQA_CONFIG(Conf);
    using BoxConstrProblem<Conf>::BoxConstrProblem;

    std::function<real_t(crvec)> f;
    std::function<void(crvec, rvec)> grad_f;
    std::function<void(crvec, rvec)> g;
    std::function<void(crvec, crvec, rvec)> grad_g_prod;
    std::function<void(crvec, index_t, rvec)> grad_gi;
    std::function<void(crvec, rmat)> jac_g;
    std::function<void(crvec, crvec, real_t, crvec, rvec)> hess_L_prod;
    std::function<void(crvec, crvec, real_t, rmat)> hess_L;
    std::function<void(crvec, crvec, crvec, real_t, crvec, rvec)> hess_ψ_prod;
    std::function<void(crvec, crvec, crvec, real_t, rmat)> hess_ψ;

    // clang-format off
    real_t eval_objective(crvec x) const { ScopedMallocAllower ma; return f(x); }
    void eval_objective_gradient(crvec x, rvec grad_fx) const { ScopedMallocAllower ma; grad_f(x, grad_fx); }
    void eval_constraints(crvec x, rvec gx) const { ScopedMallocAllower ma; g(x, gx); }
    void eval_constraints_gradient_product(crvec x, crvec y, rvec grad_gxy) const { ScopedMallocAllower ma; grad_g_prod(x, y, grad_gxy); }
    void eval_grad_gi(crvec x, index_t i, rvec grad_gix) const { ScopedMallocAllower ma; grad_gi(x, i, grad_gix); }
    void eval_lagrangian_hessian_product(crvec x, crvec y, real_t scale, crvec v, rvec Hv) const { ScopedMallocAllower ma; hess_L_prod(x, y, scale, v, Hv); }
    void eval_augmented_lagrangian_hessian_product(crvec x, crvec y, crvec Σ, real_t scale, crvec v, rvec Hv) const { ScopedMallocAllower ma; hess_ψ_prod(x, y, Σ, scale, v, Hv); }
    // clang-format on
    void eval_constraints_jacobian(crvec x, rvec J_values) const {
        ScopedMallocAllower ma;
        jac_g(x, J_values.reshaped(this->num_constraints, this->num_variables));
    }
    void eval_lagrangian_hessian(crvec x, crvec y, real_t scale, rvec H_values) const {
        ScopedMallocAllower ma;
        hess_L(x, y, scale, H_values.reshaped(this->num_variables, this->num_variables));
    }
    void eval_augmented_lagrangian_hessian(crvec x, crvec y, crvec Σ, real_t scale,
                                           rvec H_values) const {
        ScopedMallocAllower ma;
        hess_ψ(x, y, Σ, scale, H_values.reshaped(this->num_variables, this->num_variables));
    }

    /// @see @ref TypeErasedProblem::provides_eval_grad_gi
    [[nodiscard]] bool provides_eval_grad_gi() const { return bool{grad_gi}; }
    /// @see @ref TypeErasedProblem::provides_eval_constraints_jacobian
    [[nodiscard]] bool provides_eval_constraints_jacobian() const { return bool{jac_g}; }
    /// @see @ref TypeErasedProblem::provides_eval_lagrangian_hessian_product
    [[nodiscard]] bool provides_eval_lagrangian_hessian_product() const {
        return bool{hess_L_prod};
    }
    /// @see @ref TypeErasedProblem::provides_eval_lagrangian_hessian
    [[nodiscard]] bool provides_eval_lagrangian_hessian() const { return bool{hess_L}; }
    /// @see @ref TypeErasedProblem::provides_eval_augmented_lagrangian_hessian_product
    [[nodiscard]] bool provides_eval_augmented_lagrangian_hessian_product() const {
        return bool{hess_ψ_prod};
    }
    /// @see @ref TypeErasedProblem::provides_eval_augmented_lagrangian_hessian
    [[nodiscard]] bool provides_eval_augmented_lagrangian_hessian() const { return bool{hess_ψ}; }

    /// @see @ref TypeErasedProblem::get_name
    [[nodiscard]] std::string get_name() const { return "FunctionalProblem"; }

    FunctionalProblem(const FunctionalProblem &)                = default;
    FunctionalProblem &operator=(const FunctionalProblem &)     = default;
    FunctionalProblem(FunctionalProblem &&) noexcept            = default;
    FunctionalProblem &operator=(FunctionalProblem &&) noexcept = default;
};

} // namespace alpaqa
