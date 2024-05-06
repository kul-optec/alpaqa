#pragma once

#include <alpaqa/problem/type-erased-problem.hpp>
#include <stdexcept>

namespace alpaqa {

template <Config Conf>
auto ProblemVTable<Conf>::calc_ŷ_dᵀŷ(const void *self, rvec g_ŷ, crvec y, crvec Σ,
                                     const ProblemVTable &vtable) -> real_t {
    if constexpr (requires { Σ(0); })
        if (Σ.size() == 1) {
            // ζ = g(x) + Σ⁻¹y
            g_ŷ += (1 / Σ(0)) * y;
            // d = ζ - Π(ζ, D)
            vtable.eval_projecting_difference_constraints(self, g_ŷ, g_ŷ);
            // dᵀŷ, ŷ = Σ d
            real_t dᵀŷ = Σ(0) * g_ŷ.dot(g_ŷ);
            g_ŷ *= Σ(0);
            return dᵀŷ;
        }
    if (Σ.size() != y.size())
        throw std::logic_error("Penalty/multiplier size mismatch");
    // ζ = g(x) + Σ⁻¹y
    g_ŷ += y.cwiseQuotient(Σ);
    // d = ζ - Π(ζ, D)
    vtable.eval_projecting_difference_constraints(self, g_ŷ, g_ŷ);
    // dᵀŷ, ŷ = Σ d
    real_t dᵀŷ = g_ŷ.dot(Σ.cwiseProduct(g_ŷ));
    g_ŷ        = Σ.cwiseProduct(g_ŷ);
    return dᵀŷ;
}

template <Config Conf>
auto ProblemVTable<Conf>::default_eval_inactive_indices_res_lna(const void *, real_t, crvec, crvec,
                                                                rindexvec,
                                                                const ProblemVTable &) -> index_t {
    throw not_implemented_error("eval_inactive_indices_res_lna");
}

template <Config Conf>
void ProblemVTable<Conf>::default_eval_constraints_jacobian(const void *, crvec, rvec,
                                                            const ProblemVTable &vtable) {
    if (vtable.m != 0)
        throw not_implemented_error("eval_constraints_jacobian");
}

template <Config Conf>
auto ProblemVTable<Conf>::default_get_constraints_jacobian_sparsity(
    const void *, const ProblemVTable &vtable) -> Sparsity {
    return sparsity::Dense<config_t>{vtable.m, vtable.n};
}

template <Config Conf>
void ProblemVTable<Conf>::default_eval_grad_gi(const void *, crvec, index_t, rvec,
                                               const ProblemVTable &) {
    throw not_implemented_error("eval_grad_gi");
}

template <Config Conf>
void ProblemVTable<Conf>::default_eval_lagrangian_hessian_product(const void *, crvec, crvec,
                                                                  real_t, crvec, rvec,
                                                                  const ProblemVTable &) {
    throw not_implemented_error("eval_lagrangian_hessian_product");
}

template <Config Conf>
void ProblemVTable<Conf>::default_eval_lagrangian_hessian(const void *, crvec, crvec, real_t, rvec,
                                                          const ProblemVTable &) {
    throw not_implemented_error("eval_lagrangian_hessian");
}

template <Config Conf>
auto ProblemVTable<Conf>::default_get_lagrangian_hessian_sparsity(
    const void *, const ProblemVTable &vtable) -> Sparsity {
    return sparsity::Dense<config_t>{vtable.n, vtable.n, sparsity::Symmetry::Upper};
}

template <Config Conf>
void ProblemVTable<Conf>::default_eval_augmented_lagrangian_hessian_product(
    const void *self, crvec x, crvec y, crvec, real_t scale, crvec v, rvec Hv,
    const ProblemVTable &vtable) {
    if (vtable.m == 0 && vtable.eval_lagrangian_hessian_product !=
                             ProblemVTable<Conf>::default_eval_lagrangian_hessian_product)
        return vtable.eval_lagrangian_hessian_product(self, x, y, scale, v, Hv, vtable);
    throw not_implemented_error("eval_augmented_lagrangian_hessian_product");
}

template <Config Conf>
void ProblemVTable<Conf>::default_eval_augmented_lagrangian_hessian(const void *self, crvec x,
                                                                    crvec y, crvec, real_t scale,
                                                                    rvec H_values,
                                                                    const ProblemVTable &vtable) {
    if (vtable.m == 0 && vtable.eval_lagrangian_hessian != default_eval_lagrangian_hessian)
        return vtable.eval_lagrangian_hessian(self, x, y, scale, H_values, vtable);
    throw not_implemented_error("eval_augmented_lagrangian_hessian");
}

template <Config Conf>
auto ProblemVTable<Conf>::default_get_augmented_lagrangian_hessian_sparsity(
    const void *self, const ProblemVTable &vtable) -> Sparsity {
    if (vtable.m == 0 &&
        vtable.get_lagrangian_hessian_sparsity != default_get_lagrangian_hessian_sparsity)
        return vtable.get_lagrangian_hessian_sparsity(self, vtable);
    return sparsity::Dense<config_t>{vtable.n, vtable.n, sparsity::Symmetry::Upper};
}

/** @implementation{ProblemVTable<Conf>::default_eval_objective_and_gradient} */
template <Config Conf>
/* [ProblemVTable<Conf>::default_eval_objective_and_gradient] */
auto ProblemVTable<Conf>::default_eval_objective_and_gradient(
    const void *self, crvec x, rvec grad_fx, const ProblemVTable &vtable) -> real_t {
    vtable.eval_objective_gradient(self, x, grad_fx);
    return vtable.eval_objective(self, x);
}
/* [ProblemVTable<Conf>::default_eval_objective_and_gradient] */

/** @implementation{ProblemVTable<Conf>::default_eval_objective_and_constraints} */
template <Config Conf>
/* [ProblemVTable<Conf>::default_eval_objective_and_constraints] */
auto ProblemVTable<Conf>::default_eval_objective_and_constraints(
    const void *self, crvec x, rvec g, const ProblemVTable &vtable) -> real_t {
    vtable.eval_constraints(self, x, g);
    return vtable.eval_objective(self, x);
}
/* [ProblemVTable<Conf>::default_eval_objective_and_constraints] */

/** @implementation{ProblemVTable<Conf>::default_eval_objective_gradient_and_constraints_gradient_product} */
template <Config Conf>
/* [ProblemVTable<Conf>::default_eval_objective_gradient_and_constraints_gradient_product] */
void ProblemVTable<Conf>::default_eval_objective_gradient_and_constraints_gradient_product(
    const void *self, crvec x, crvec y, rvec grad_f, rvec grad_gxy, const ProblemVTable &vtable) {
    vtable.eval_objective_gradient(self, x, grad_f);
    vtable.eval_constraints_gradient_product(self, x, y, grad_gxy);
}
/* [ProblemVTable<Conf>::default_eval_objective_gradient_and_constraints_gradient_product] */

/** @implementation{ProblemVTable<Conf>::default_eval_lagrangian_gradient} */
template <Config Conf>
/* [ProblemVTable<Conf>::default_eval_lagrangian_gradient] */
void ProblemVTable<Conf>::default_eval_lagrangian_gradient(const void *self, crvec x, crvec y,
                                                           rvec grad_L, rvec work_n,
                                                           const ProblemVTable &vtable) {
    if (y.size() == 0) /* [[unlikely]] */
        return vtable.eval_objective_gradient(self, x, grad_L);
    vtable.eval_objective_gradient_and_constraints_gradient_product(self, x, y, grad_L, work_n,
                                                                    vtable);
    grad_L += work_n;
}
/* [ProblemVTable<Conf>::default_eval_lagrangian_gradient] */

/** @implementation{ProblemVTable<Conf>::default_eval_augmented_lagrangian} */
template <Config Conf>
/* [ProblemVTable<Conf>::default_eval_augmented_lagrangian] */
auto ProblemVTable<Conf>::default_eval_augmented_lagrangian(const void *self, crvec x, crvec y,
                                                            crvec Σ, rvec ŷ,
                                                            const ProblemVTable &vtable) -> real_t {
    if (y.size() == 0) /* [[unlikely]] */
        return vtable.eval_objective(self, x);

    auto f   = vtable.eval_objective_and_constraints(self, x, ŷ, vtable);
    auto dᵀŷ = calc_ŷ_dᵀŷ(self, ŷ, y, Σ, vtable);
    // ψ(x) = f(x) + ½ dᵀŷ
    auto ψ = f + real_t(0.5) * dᵀŷ;
    return ψ;
}
/* [ProblemVTable<Conf>::default_eval_augmented_lagrangian] */

/** @implementation{ProblemVTable<Conf>::default_eval_augmented_lagrangian_gradient} */
template <Config Conf>
/* [ProblemVTable<Conf>::default_eval_augmented_lagrangian_gradient] */
void ProblemVTable<Conf>::default_eval_augmented_lagrangian_gradient(const void *self, crvec x,
                                                                     crvec y, crvec Σ, rvec grad_ψ,
                                                                     rvec work_n, rvec work_m,
                                                                     const ProblemVTable &vtable) {
    if (y.size() == 0) /* [[unlikely]] */ {
        vtable.eval_objective_gradient(self, x, grad_ψ);
    } else {
        vtable.eval_constraints(self, x, work_m);
        (void)calc_ŷ_dᵀŷ(self, work_m, y, Σ, vtable);
        vtable.eval_lagrangian_gradient(self, x, work_m, grad_ψ, work_n, vtable);
    }
}
/* [ProblemVTable<Conf>::default_eval_augmented_lagrangian_gradient] */

/** @implementation{ProblemVTable<Conf>::default_eval_augmented_lagrangian_and_gradient} */
template <Config Conf>
/* [ProblemVTable<Conf>::default_eval_augmented_lagrangian_and_gradient] */
auto ProblemVTable<Conf>::default_eval_augmented_lagrangian_and_gradient(
    const void *self, crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m,
    const ProblemVTable &vtable) -> real_t {
    if (y.size() == 0) /* [[unlikely]] */
        return vtable.eval_objective_and_gradient(self, x, grad_ψ, vtable);

    auto &ŷ = work_m;
    // ψ(x) = f(x) + ½ dᵀŷ
    auto f   = vtable.eval_objective_and_constraints(self, x, ŷ, vtable);
    auto dᵀŷ = calc_ŷ_dᵀŷ(self, ŷ, y, Σ, vtable);
    auto ψ   = f + real_t(0.5) * dᵀŷ;
    // ∇ψ(x) = ∇f(x) + ∇g(x) ŷ
    vtable.eval_lagrangian_gradient(self, x, ŷ, grad_ψ, work_n, vtable);
    return ψ;
}
/* [ProblemVTable<Conf>::default_eval_augmented_lagrangian_and_gradient] */

template <Config Conf>
auto ProblemVTable<Conf>::default_get_box_variables(const void *,
                                                    const ProblemVTable &) -> const Box & {
    throw not_implemented_error("get_box_variables");
}

template <Config Conf>
auto ProblemVTable<Conf>::default_get_box_general_constraints(const void *, const ProblemVTable &)
    -> const Box & {
    throw not_implemented_error("get_box_general_constraints");
}

template <Config Conf>
void ProblemVTable<Conf>::default_check(const void *, const ProblemVTable &) {}

template <Config Conf>
std::string ProblemVTable<Conf>::default_get_name(const void *, const ProblemVTable &) {
    return "unknown problem";
}

} // namespace alpaqa