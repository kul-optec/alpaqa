#pragma once

#include <alpaqa/problem/problem-counters.hpp>
#include <alpaqa/problem/sparsity.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>
#include <alpaqa/util/timed.hpp>

#include <type_traits>

namespace alpaqa {

/// @addtogroup grp_Problems
/// @{

/// Problem wrapper that keeps track of the number of evaluations and the run
/// time of each function.
/// You probably want to use @ref problem_with_counters or
/// @ref problem_with_counters_ref instead of instantiating this class directly.
/// @note   The evaluation counters are stored using a `std::shared_pointers`,
///         which means that different copies of a @ref ProblemWithCounters
///         instance all share the same counters. To opt out of this behavior,
///         you can use the @ref decouple_evaluations function.
template <class Problem>
struct ProblemWithCounters {
    USING_ALPAQA_CONFIG_TEMPLATE(std::remove_cvref_t<Problem>::config_t);
    using Box      = typename TypeErasedProblem<config_t>::Box;
    using Sparsity = sparsity::Sparsity<config_t>;

    // clang-format off
    [[gnu::always_inline]] void eval_projecting_difference_constraints(crvec z, rvec e) const { ++evaluations->proj_diff_g; return timed(evaluations->time.proj_diff_g, [&] { return problem.eval_projecting_difference_constraints(z, e); }); }
    [[gnu::always_inline]] void eval_projection_multipliers(rvec y, real_t M) const { ++evaluations->proj_multipliers; return timed(evaluations->time.proj_multipliers, [&] { return problem.eval_projection_multipliers(y, M); }); }
    [[gnu::always_inline]] real_t eval_proximal_gradient_step(real_t γ, crvec x, crvec grad_ψ, rvec x̂, rvec p) const { ++evaluations->prox_grad_step; return timed(evaluations->time.prox_grad_step, [&] { return problem.eval_proximal_gradient_step(γ, x, grad_ψ, x̂, p); }); }
    [[gnu::always_inline]] index_t eval_inactive_indices_res_lna(real_t γ, crvec x, crvec grad_ψ, rindexvec J) const requires requires { &std::remove_cvref_t<Problem>::eval_inactive_indices_res_lna; } { ++evaluations->inactive_indices_res_lna; return timed(evaluations->time.inactive_indices_res_lna, [&] { return problem.eval_inactive_indices_res_lna(γ, x, grad_ψ, J); }); }
    [[gnu::always_inline]] real_t eval_objective(crvec x) const { ++evaluations->f; return timed(evaluations->time.f, [&] { return problem.eval_objective(x); }); }
    [[gnu::always_inline]] void eval_objective_gradient(crvec x, rvec grad_fx) const { ++evaluations->grad_f; return timed(evaluations->time.grad_f, [&] { return problem.eval_objective_gradient(x, grad_fx); }); }
    [[gnu::always_inline]] void eval_constraints(crvec x, rvec gx) const { ++evaluations->g; return timed(evaluations->time.g, [&] { return problem.eval_constraints(x, gx); }); }
    [[gnu::always_inline]] void eval_constraints_gradient_product(crvec x, crvec y, rvec grad_gxy) const { ++evaluations->grad_g_prod; return timed(evaluations->time.grad_g_prod, [&] { return problem.eval_constraints_gradient_product(x, y, grad_gxy); }); }
    [[gnu::always_inline]] void eval_grad_gi(crvec x, index_t i, rvec grad_gi) const requires requires { &std::remove_cvref_t<Problem>::eval_grad_gi; } { ++evaluations->grad_gi; return timed(evaluations->time.grad_gi, [&] { return problem.eval_grad_gi(x, i, grad_gi); }); }
    [[gnu::always_inline]] void eval_constraints_jacobian(crvec x, rvec J_values) const requires requires { &std::remove_cvref_t<Problem>::eval_constraints_jacobian; } { ++evaluations->jac_g; return timed(evaluations->time.jac_g, [&] { return problem.eval_constraints_jacobian(x, J_values); }); }
    [[gnu::always_inline]] Sparsity get_constraints_jacobian_sparsity() const requires requires { &std::remove_cvref_t<Problem>::get_constraints_jacobian_sparsity; } { return problem.get_constraints_jacobian_sparsity(); }
    [[gnu::always_inline]] void eval_lagrangian_hessian_product(crvec x, crvec y, real_t scale, crvec v, rvec Hv) const requires requires { &std::remove_cvref_t<Problem>::eval_lagrangian_hessian_product; } { ++evaluations->hess_L_prod; return timed(evaluations->time.hess_L_prod, [&] { return problem.eval_lagrangian_hessian_product(x, y, scale, v, Hv); }); }
    [[gnu::always_inline]] void eval_lagrangian_hessian(crvec x, crvec y, real_t scale, rvec H_values) const requires requires { &std::remove_cvref_t<Problem>::eval_lagrangian_hessian; } { ++evaluations->hess_L; return timed(evaluations->time.hess_L, [&] { return problem.eval_lagrangian_hessian(x, y, scale, H_values); }); }
    [[gnu::always_inline]] Sparsity get_lagrangian_hessian_sparsity() const requires requires { &std::remove_cvref_t<Problem>::get_lagrangian_hessian_sparsity; } { return problem.get_lagrangian_hessian_sparsity(); }
    [[gnu::always_inline]] void eval_augmented_lagrangian_hessian_product(crvec x, crvec y, crvec Σ, real_t scale, crvec v, rvec Hv) const requires requires { &std::remove_cvref_t<Problem>::eval_augmented_lagrangian_hessian_product; } { ++evaluations->hess_ψ_prod; return timed(evaluations->time.hess_ψ_prod, [&] { return problem.eval_augmented_lagrangian_hessian_product(x, y, Σ, scale, v, Hv); }); }
    [[gnu::always_inline]] void eval_augmented_lagrangian_hessian(crvec x, crvec y, crvec Σ, real_t scale, rvec H_values) const requires requires { &std::remove_cvref_t<Problem>::eval_augmented_lagrangian_hessian; } { ++evaluations->hess_ψ; return timed(evaluations->time.hess_ψ, [&] { return problem.eval_augmented_lagrangian_hessian(x, y, Σ, scale, H_values); }); }
    [[gnu::always_inline]] Sparsity get_augmented_lagrangian_hessian_sparsity() const requires requires { &std::remove_cvref_t<Problem>::get_augmented_lagrangian_hessian_sparsity; } { return problem.get_augmented_lagrangian_hessian_sparsity(); }
    [[gnu::always_inline]] real_t eval_objective_and_gradient(crvec x, rvec grad_fx) const requires requires { &std::remove_cvref_t<Problem>::eval_objective_and_gradient; } { ++evaluations->f_grad_f; return timed(evaluations->time.f_grad_f, [&] { return problem.eval_objective_and_gradient(x, grad_fx); }); }
    [[gnu::always_inline]] real_t eval_objective_and_constraints(crvec x, rvec g) const requires requires { &std::remove_cvref_t<Problem>::eval_objective_and_constraints; } { ++evaluations->f_g; return timed(evaluations->time.f_g, [&] { return problem.eval_objective_and_constraints(x, g); }); }
    [[gnu::always_inline]] void eval_objective_gradient_and_constraints_gradient_product(crvec x, crvec y, rvec grad_f, rvec grad_gxy) const requires requires { &std::remove_cvref_t<Problem>::eval_objective_gradient_and_constraints_gradient_product; } { ++evaluations->grad_f_grad_g_prod; return timed(evaluations->time.grad_f_grad_g_prod, [&] { return problem.eval_objective_gradient_and_constraints_gradient_product(x, y, grad_f, grad_gxy); }); }
    [[gnu::always_inline]] void eval_lagrangian_gradient(crvec x, crvec y, rvec grad_L, rvec work_n) const requires requires { &std::remove_cvref_t<Problem>::eval_lagrangian_gradient; } { ++evaluations->grad_L; return timed(evaluations->time.grad_L, [&] { return problem.eval_lagrangian_gradient(x, y, grad_L, work_n); }); }
    [[gnu::always_inline]] real_t eval_augmented_lagrangian(crvec x, crvec y, crvec Σ, rvec ŷ) const requires requires { &std::remove_cvref_t<Problem>::eval_augmented_lagrangian; } { ++evaluations->ψ; return timed(evaluations->time.ψ, [&] { return problem.eval_augmented_lagrangian(x, y, Σ, ŷ); }); }
    [[gnu::always_inline]] void eval_augmented_lagrangian_gradient(crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m) const requires requires { &std::remove_cvref_t<Problem>::eval_augmented_lagrangian_gradient; } { ++evaluations->grad_ψ; return timed(evaluations->time.grad_ψ, [&] { return problem.eval_augmented_lagrangian_gradient(x, y, Σ, grad_ψ, work_n, work_m); }); }
    [[gnu::always_inline]] real_t eval_augmented_lagrangian_and_gradient(crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m) const requires requires { &std::remove_cvref_t<Problem>::eval_augmented_lagrangian_and_gradient; } { ++evaluations->ψ_grad_ψ; return timed(evaluations->time.ψ_grad_ψ, [&] { return problem.eval_augmented_lagrangian_and_gradient(x, y, Σ, grad_ψ, work_n, work_m); }); }
    const Box &get_box_variables() const requires requires { &std::remove_cvref_t<Problem>::get_box_variables; } { return problem.get_box_variables(); }
    const Box &get_box_general_constraints() const requires requires { &std::remove_cvref_t<Problem>::get_box_general_constraints; } { return problem.get_box_general_constraints(); }
    void check() const requires requires { &std::remove_cvref_t<Problem>::check; } { return problem.check(); }
    [[nodiscard]] std::string get_name() const requires requires { &std::remove_cvref_t<Problem>::get_name; } { return problem.get_name(); }

    [[nodiscard]] bool provides_eval_grad_gi() const requires requires (Problem p) { { p.provides_eval_grad_gi() } -> std::convertible_to<bool>; } { return problem.provides_eval_grad_gi(); }
    [[nodiscard]] bool provides_eval_inactive_indices_res_lna() const requires requires (Problem p) { { p.provides_eval_inactive_indices_res_lna() } -> std::convertible_to<bool>; } { return problem.provides_eval_inactive_indices_res_lna(); }
    [[nodiscard]] bool provides_eval_constraints_jacobian() const requires requires (Problem p) { { p.provides_eval_constraints_jacobian() } -> std::convertible_to<bool>; } { return problem.provides_eval_constraints_jacobian(); }
    [[nodiscard]] bool provides_get_constraints_jacobian_sparsity() const requires requires (Problem p) { { p.provides_get_constraints_jacobian_sparsity() } -> std::convertible_to<bool>; } { return problem.provides_get_constraints_jacobian_sparsity(); }
    [[nodiscard]] bool provides_eval_lagrangian_hessian_product() const requires requires (Problem p) { { p.provides_eval_lagrangian_hessian_product() } -> std::convertible_to<bool>; } { return problem.provides_eval_lagrangian_hessian_product(); }
    [[nodiscard]] bool provides_eval_lagrangian_hessian() const requires requires (Problem p) { { p.provides_eval_lagrangian_hessian() } -> std::convertible_to<bool>; } { return problem.provides_eval_lagrangian_hessian(); }
    [[nodiscard]] bool provides_get_lagrangian_hessian_sparsity() const requires requires (Problem p) { { p.provides_get_lagrangian_hessian_sparsity() } -> std::convertible_to<bool>; } { return problem.provides_get_lagrangian_hessian_sparsity(); }
    [[nodiscard]] bool provides_eval_augmented_lagrangian_hessian_product() const requires requires (Problem p) { { p.provides_eval_augmented_lagrangian_hessian() } -> std::convertible_to<bool>; } { return problem.provides_eval_augmented_lagrangian_hessian_product(); }
    [[nodiscard]] bool provides_eval_augmented_lagrangian_hessian() const requires requires (Problem p) { { p.provides_eval_augmented_lagrangian_hessian() } -> std::convertible_to<bool>; } { return problem.provides_eval_augmented_lagrangian_hessian(); }
    [[nodiscard]] bool provides_get_augmented_lagrangian_hessian_sparsity() const requires requires (Problem p) { { p.provides_get_augmented_lagrangian_hessian_sparsity() } -> std::convertible_to<bool>; } { return problem.provides_get_augmented_lagrangian_hessian_sparsity(); }
    [[nodiscard]] bool provides_eval_objective_and_gradient() const requires requires (Problem p) { { p.provides_eval_objective_and_gradient() } -> std::convertible_to<bool>; } { return problem.provides_eval_objective_and_gradient(); }
    [[nodiscard]] bool provides_eval_objective_and_constraints() const requires requires (Problem p) { { p.provides_eval_objective_and_constraints() } -> std::convertible_to<bool>; } { return problem.provides_eval_objective_and_constraints(); }
    [[nodiscard]] bool provides_eval_objective_gradient_and_constraints_gradient_product() const requires requires (Problem p) { { p.provides_eval_objective_gradient_and_constraints_gradient_product() } -> std::convertible_to<bool>; } { return problem.provides_eval_objective_gradient_and_constraints_gradient_product(); }
    [[nodiscard]] bool provides_eval_lagrangian_gradient() const requires requires (Problem p) { { p.provides_eval_lagrangian_gradient() } -> std::convertible_to<bool>; } { return problem.provides_eval_lagrangian_gradient(); }
    [[nodiscard]] bool provides_eval_augmented_lagrangian() const requires requires (Problem p) { { p.provides_eval_augmented_lagrangian() } -> std::convertible_to<bool>; } { return problem.provides_eval_augmented_lagrangian(); }
    [[nodiscard]] bool provides_eval_augmented_lagrangian_gradient() const requires requires (Problem p) { { p.provides_eval_augmented_lagrangian_gradient() } -> std::convertible_to<bool>; } { return problem.provides_eval_augmented_lagrangian_gradient(); }
    [[nodiscard]] bool provides_eval_augmented_lagrangian_and_gradient() const requires requires (Problem p) { { p.provides_eval_augmented_lagrangian_and_gradient() } -> std::convertible_to<bool>; } { return problem.provides_eval_augmented_lagrangian_and_gradient(); }
    [[nodiscard]] bool provides_get_box_variables() const requires requires (Problem p) { { p.provides_get_box_variables() } -> std::convertible_to<bool>; } { return problem.provides_get_box_variables(); }
    [[nodiscard]] bool provides_get_box_general_constraints() const requires requires (Problem p) { { p.provides_get_box_general_constraints() } -> std::convertible_to<bool>; } { return problem.provides_get_box_general_constraints(); }
    [[nodiscard]] bool provides_check() const requires requires (Problem p) { { p.provides_check() } -> std::convertible_to<bool>; } { return problem.provides_check(); }
    [[nodiscard]] bool provides_get_name() const requires requires (Problem p) { { p.provides_get_name() } -> std::convertible_to<bool>; } { return problem.provides_get_name(); }
    // clang-format on

    [[nodiscard]] length_t get_num_variables() const { return problem.get_num_variables(); }
    [[nodiscard]] length_t get_num_constraints() const { return problem.get_num_constraints(); }

    std::shared_ptr<EvalCounter> evaluations = std::make_shared<EvalCounter>();
    Problem problem;

    ProblemWithCounters()
        requires std::is_default_constructible_v<Problem>
    = default;
    template <class P>
    explicit ProblemWithCounters(P &&problem)
        requires std::is_same_v<std::remove_cvref_t<P>, std::remove_cvref_t<Problem>>
        : problem{std::forward<P>(problem)} {}
    template <class... Args>
    explicit ProblemWithCounters(std::in_place_t, Args &&...args)
        requires(!std::is_lvalue_reference_v<Problem>)
        : problem{std::forward<Args>(args)...} {}

    /// Reset all evaluation counters and timers to zero. Affects all instances
    /// that share the same evaluations. If you only want to reset the counters
    /// of this instance, use @ref decouple_evaluations first.
    void reset_evaluations() { evaluations.reset(); }
    /// Give this instance its own evaluation counters and timers, decoupling
    /// it from any other instances they might have previously been shared with.
    /// The evaluation counters and timers are preserved (a copy is made).
    void decouple_evaluations() { evaluations = std::make_shared<EvalCounter>(*evaluations); }

  private:
    template <class TimeT, class FunT>
    [[gnu::always_inline]] static decltype(auto) timed(TimeT &time, FunT &&f) {
        util::Timed timed{time};
        return std::forward<FunT>(f)();
    }
};

/// Wraps the given problem into a @ref ProblemWithCounters and keeps track of
/// how many times each function is called, and how long these calls took.
/// The wrapper has its own copy of the given problem. Making copies of the
/// wrapper also copies the underlying problem, but does not copy the evaluation
/// counters, all copies share the same counters.
template <class Problem>
[[nodiscard]] auto problem_with_counters(Problem &&p) {
    using Prob        = std::remove_cvref_t<Problem>;
    using ProbWithCnt = ProblemWithCounters<Prob>;
    return ProbWithCnt{std::forward<Problem>(p)};
}

/// Wraps the given problem into a @ref ProblemWithCounters and keeps track of
/// how many times each function is called, and how long these calls took.
/// The wrapper keeps only a reference to the given problem, it is the
/// responsibility of the caller to make sure that the wrapper does not outlive
/// the original problem. Making copies of the wrapper does not copy the
/// evaluation counters, all copies share the same counters.
template <class Problem>
[[nodiscard]] auto problem_with_counters_ref(Problem &p) {
    using Prob        = std::remove_cvref_t<Problem>;
    using ProbWithCnt = ProblemWithCounters<const Prob &>;
    return ProbWithCnt{p};
}

/// @}

} // namespace alpaqa
