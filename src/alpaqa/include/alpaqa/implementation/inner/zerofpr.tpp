#pragma once

#include <alpaqa/inner/zerofpr.hpp>

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include <alpaqa/config/config.hpp>
#include <alpaqa/implementation/inner/panoc-helpers.tpp>
#include <alpaqa/implementation/util/print.tpp>
#include <alpaqa/util/alloc-check.hpp>
#include <alpaqa/util/quadmath/quadmath-print.hpp>

namespace alpaqa {

template <class DirectionProviderT>
std::string ZeroFPRSolver<DirectionProviderT>::get_name() const {
    return "ZeroFPRSolver<" + std::string(direction.get_name()) + ">";
}

template <class DirectionProviderT>
auto ZeroFPRSolver<DirectionProviderT>::operator()(
    /// [in]    Problem description
    const Problem &problem,
    /// [in]    Solve options
    const SolveOptions &opts,
    /// [inout] Decision variable @f$ x @f$
    rvec x,
    /// [inout] Lagrange multipliers @f$ y @f$
    rvec y,
    /// [in]    Constraint weights @f$ \Sigma @f$
    crvec Σ,
    /// [out]   Slack variable error @f$ g(x) - \Pi_D(g(x) + \Sigma^{-1} y) @f$
    rvec err_z) -> Stats {

    using std::chrono::nanoseconds;
    auto os         = opts.os ? opts.os : this->os;
    auto start_time = std::chrono::steady_clock::now();
    Stats s;

    const auto n = problem.get_n();
    const auto m = problem.get_m();

    // Represents an intermediate proximal iterate in the algorithm.
    struct ProxIterate {
        vec x̂;      //< Decision variables after proximal gradient step
        vec grad_ψ; //< Gradient of cost in x
        vec p;      //< Proximal gradient step in x
        vec ŷx̂;     //< Candidate Lagrange multipliers in x̂
        real_t pᵀp      = NaN<config_t>; //< Norm squared of p
        real_t grad_ψᵀp = NaN<config_t>; //< Dot product of gradient and p
        real_t hx̂       = NaN<config_t>; //< Non-smooth function value in x̂

        ProxIterate(length_t n, length_t m) : x̂(n), grad_ψ(n), p(n), ŷx̂(m) {}
    } prox_iterate{n, m};
    // Represents an iterate in the algorithm, keeping track of some
    // intermediate values and function evaluations.
    struct Iterate {
        vec x;      //< Decision variables
        vec x̂;      //< Decision variables after proximal gradient step
        vec grad_ψ; //< Gradient of cost in x
        vec p;      //< Proximal gradient step in x
        vec ŷx̂;     //< Candidate Lagrange multipliers in x̂
        real_t ψx       = NaN<config_t>; //< Cost in x
        real_t ψx̂       = NaN<config_t>; //< Cost in x̂
        real_t γ        = NaN<config_t>; //< Step size γ
        real_t L        = NaN<config_t>; //< Lipschitz estimate L
        real_t pᵀp      = NaN<config_t>; //< Norm squared of p
        real_t grad_ψᵀp = NaN<config_t>; //< Dot product of gradient and p
        real_t hx̂       = NaN<config_t>; //< Non-smooth function value in x̂

        // @pre    @ref ψx, @ref hx̂ @ref pᵀp, @ref grad_ψᵀp
        // @return φγ
        real_t fbe() const { return ψx + hx̂ + pᵀp / (2 * γ) + grad_ψᵀp; }

        Iterate(length_t n, length_t m) : x(n), x̂(n), grad_ψ(n), p(n), ŷx̂(m) {}
    } iterates[2]{{n, m}, {n, m}};
    Iterate *curr     = &iterates[0];
    ProxIterate *prox = &prox_iterate;
    Iterate *next     = &iterates[1];

    vec work_n(n), work_m(m);
    vec q(n); // (quasi-)Newton step Hₖ pₖ

    // Helper functions --------------------------------------------------------

    auto qub_violated = [this](const Iterate &i) {
        real_t margin =
            (1 + std::abs(i.ψx)) * params.quadratic_upperbound_tolerance_factor;
        return i.ψx̂ > i.ψx + i.grad_ψᵀp + real_t(0.5) * i.L * i.pᵀp + margin;
    };

    auto linesearch_violated = [this](const Iterate &curr,
                                      const Iterate &next) {
        if (params.force_linesearch)
            return false;
        real_t σ  = params.β * (1 - curr.γ * curr.L) / (2 * curr.γ);
        real_t φγ = curr.fbe();
        real_t margin = (1 + std::abs(φγ)) * params.linesearch_tolerance_factor;
        return next.fbe() > φγ - σ * curr.pᵀp + margin;
    };

    // Problem functions -------------------------------------------------------

    auto eval_ψ_grad_ψ = [&problem, &y, &Σ, &work_n, &work_m](Iterate &i) {
        i.ψx = problem.eval_ψ_grad_ψ(i.x, y, Σ, i.grad_ψ, work_n, work_m);
    };
    auto eval_prox_grad_step = [&problem](Iterate &i) {
        i.hx̂  = problem.eval_prox_grad_step(i.γ, i.x, i.grad_ψ, i.x̂, i.p);
        i.pᵀp = i.p.squaredNorm();
        i.grad_ψᵀp = i.p.dot(i.grad_ψ);
    };
    auto eval_cost_in_prox = [&problem, &y, &Σ](Iterate &i) {
        i.ψx̂ = problem.eval_ψ(i.x̂, y, Σ, i.ŷx̂);
    };
    auto eval_grad_in_prox = [&problem, &prox, &work_n](const Iterate &i) {
        problem.eval_grad_L(i.x̂, i.ŷx̂, prox->grad_ψ, work_n);
    };
    auto eval_prox_grad_step_in_prox = [&problem, &prox](const Iterate &i) {
        prox->hx̂ = problem.eval_prox_grad_step(i.γ, i.x̂, prox->grad_ψ, prox->x̂,
                                               prox->p);
        prox->pᵀp      = prox->p.squaredNorm();
        prox->grad_ψᵀp = prox->p.dot(prox->grad_ψ);
    };

    // Printing ----------------------------------------------------------------

    std::array<char, 64> print_buf;
    auto print_real = [this, &print_buf](real_t x) {
        return float_to_str_vw(print_buf, x, params.print_precision);
    };
    auto print_real3 = [&print_buf](real_t x) {
        return float_to_str_vw(print_buf, x, 3);
    };
    auto print_progress = [&](unsigned k, real_t φγ, real_t ψ, crvec grad_ψ,
                              real_t pᵀp, crvec q, real_t γ, real_t τ,
                              real_t ε) {
        *os << "[ZeroFPR] " << std::setw(6) << k << ": φγ = " << print_real(φγ)
            << ", ψ = " << print_real(ψ)
            << ", ‖∇ψ‖ = " << print_real(grad_ψ.norm())
            << ", ‖p‖ = " << print_real(std::sqrt(pᵀp))
            << ", γ = " << print_real(γ) << ", ε = " << print_real(ε);
        if (k > 0)
            *os << ", τ = " << print_real3(τ)
                << ", ‖q‖ = " << print_real(q.norm());
        *os << std::endl; // Flush for Python buffering
    };

    // Initialization ----------------------------------------------------------

    curr->x = x;

    // Estimate Lipschitz constant ---------------------------------------------

    // Finite difference approximation of ∇²ψ in starting point
    if (params.Lipschitz.L_0 <= 0) {
        curr->L = Helpers::initial_lipschitz_estimate(
            problem, curr->x, y, Σ, params.Lipschitz.ε, params.Lipschitz.δ,
            params.L_min, params.L_max,
            /* in ⟹ out */ curr->ψx, curr->grad_ψ, curr->x̂, next->grad_ψ,
            work_n, work_m);
    }
    // Initial Lipschitz constant provided by the user
    else {
        curr->L = params.Lipschitz.L_0;
        // Calculate ψ(xₖ), ∇ψ(x₀)
        eval_ψ_grad_ψ(*curr);
    }
    if (not std::isfinite(curr->L)) {
        s.status = SolverStatus::NotFinite;
        return s;
    }
    curr->γ = params.Lipschitz.Lγ_factor / curr->L;

    // First proximal gradient step --------------------------------------------

    // Calculate x̂ₖ, ψ(x̂ₖ)
    eval_prox_grad_step(*curr);
    eval_cost_in_prox(*curr);

    // Quadratic upper bound
    while (curr->L < params.L_max && qub_violated(*curr)) {
        curr->γ /= 2;
        curr->L *= 2;
        eval_prox_grad_step(*curr);
        eval_cost_in_prox(*curr);
    }

    // Loop data ---------------------------------------------------------------

    unsigned k = 0;             // iteration
    real_t τ   = NaN<config_t>; // line search parameter
    // Keep track of how many successive iterations didn't update the iterate
    unsigned no_progress = 0;

    // Main ZeroFPR loop
    // =========================================================================

    ScopedMallocBlocker mb; // Don't allocate in the inner loop
    while (true) {

        // Check stopping criteria ---------------------------------------------

        // Calculate ∇ψ(x̂ₖ), p̂ₖ
        eval_grad_in_prox(*curr);
        eval_prox_grad_step_in_prox(*curr);

        real_t εₖ = Helpers::calc_error_stop_crit(
            problem, params.stop_crit, curr->p, curr->γ, curr->x, curr->x̂,
            curr->ŷx̂, curr->grad_ψ, prox->grad_ψ, work_n, next->p);

        // Print progress ------------------------------------------------------

        if (params.print_interval != 0 && k % params.print_interval == 0)
            print_progress(k, curr->fbe(), curr->ψx, curr->grad_ψ, curr->pᵀp, q,
                           curr->γ, τ, εₖ);
        if (progress_cb) {
            ScopedMallocAllower ma;
            progress_cb({.k          = k,
                         .x          = curr->x,
                         .p          = curr->p,
                         .norm_sq_p  = curr->pᵀp,
                         .x̂          = curr->x̂,
                         .φγ         = curr->fbe(),
                         .ψ          = curr->ψx,
                         .grad_ψ     = curr->grad_ψ,
                         .ψ_hat      = curr->ψx̂,
                         .grad_ψ_hat = prox->grad_ψ,
                         .q          = q,
                         .L          = curr->L,
                         .γ          = curr->γ,
                         .τ          = τ,
                         .ε          = εₖ,
                         .Σ          = Σ,
                         .y          = y,
                         .problem    = problem,
                         .params     = params});
        }

        // Return solution -----------------------------------------------------

        auto time_elapsed = std::chrono::steady_clock::now() - start_time;
        auto stop_status  = Helpers::check_all_stop_conditions(
            params, opts, time_elapsed, k, stop_signal, εₖ, no_progress);
        if (stop_status != SolverStatus::Busy) {
            // TODO: move the computation of ẑ and g(x) to ALM?
            if (stop_status == SolverStatus::Converged ||
                stop_status == SolverStatus::Interrupted ||
                opts.always_overwrite_results) {
                auto &ŷ = curr->ŷx̂;
                if (err_z.size() > 0)
                    err_z = Σ.asDiagonal().inverse() * (ŷ - y);
                x = std::move(curr->x̂);
                y = std::move(curr->ŷx̂);
            }
            s.iterations   = k;
            s.ε            = εₖ;
            s.elapsed_time = duration_cast<nanoseconds>(time_elapsed);
            s.status       = stop_status;
            s.final_γ      = curr->γ;
            s.final_ψ      = curr->ψx̂;
            s.final_h      = curr->hx̂;
            s.final_φγ     = curr->fbe();
            return s;
        }

        // Calculate quasi-Newton step -----------------------------------------

        real_t τ_init = NaN<config_t>;
        if (k == 0) { // Initialize L-BFGS
            ScopedMallocAllower ma;
            direction.initialize(problem, y, Σ, curr->γ, curr->x̂, prox->x̂,
                                 prox->p, prox->grad_ψ);
            τ_init = 0;
        }
        if (k > 0 || direction.has_initial_direction()) {
            τ_init = direction.apply(curr->γ, curr->x̂, prox->x̂, prox->p,
                                     prox->grad_ψ, q)
                         ? 1
                         : 0;
            // Make sure quasi-Newton step is valid
            if (τ_init == 1 && not q.allFinite())
                τ_init = 0;
            if (τ_init != 1) { // If we computed a quasi-Newton step
                ++s.lbfgs_failures;
                direction.reset(); // Is there anything else we can do?
            }
        }

        // Line search ---------------------------------------------------------

        next->γ                         = curr->γ;
        next->L                         = curr->L;
        τ                               = τ_init;
        real_t τ_prev                   = -1;
        bool update_lbfgs_in_linesearch = params.update_direction_in_candidate;
        bool update_lbfgs_later         = !update_lbfgs_in_linesearch;

        // xₖ₊₁ = xₖ + pₖ
        auto take_safe_step = [&] {
            next->x      = curr->x̂; // → safe prox step
            next->ψx     = curr->ψx̂;
            next->grad_ψ = prox->grad_ψ;
            // TODO: could swap gradients, but need for direction update
        };

        // xₖ₊₁ = x̂ₖ + τ qₖ
        auto take_accelerated_step = [&](real_t τ) {
            if (τ == 1) // → faster quasi-Newton step
                next->x = curr->x̂ + q;
            else
                next->x = curr->x̂ + τ * q;
            // Calculate ψ(xₖ₊₁), ∇ψ(xₖ₊₁)
            eval_ψ_grad_ψ(*next);
        };

        while (!stop_signal.stop_requested()) {

            // Recompute step only if τ changed
            if (τ != τ_prev) {
                τ != 0 ? take_accelerated_step(τ) : take_safe_step();
                τ_prev = τ;
            }

            // If the cost is not finite, abandon the direction entirely, don't
            // even bother backtracking.
            if (τ > 0 && !std::isfinite(next->ψx)) {
                τ = 0;
                direction.reset();
                continue;
            }

            // Calculate x̂ₖ₊₁, ψ(x̂ₖ₊₁)
            eval_prox_grad_step(*next);
            eval_cost_in_prox(*next);

            // Quadratic upper bound
            if (next->L < params.L_max && qub_violated(*next)) {
                next->γ /= 2;
                next->L *= 2;
                τ = τ_init;
                ++s.stepsize_backtracks;
                update_lbfgs_in_linesearch = false;
                update_lbfgs_later         = true;
                continue;
            }

            // Update L-BFGS
            if (τ == 1 && update_lbfgs_in_linesearch) {
                if (params.update_direction_from_prox_step) {
                    s.lbfgs_rejected += not direction.update(
                        curr->γ, next->γ, curr->x̂, next->x, prox->p, next->p,
                        prox->grad_ψ, next->grad_ψ);
                } else {
                    s.lbfgs_rejected += not direction.update(
                        curr->γ, next->γ, curr->x, next->x, curr->p, next->p,
                        curr->grad_ψ, next->grad_ψ);
                }
                update_lbfgs_in_linesearch = false;
                update_lbfgs_later         = false;
            }

            // Line search condition
            if (τ > 0 && linesearch_violated(*curr, *next)) {
                τ /= 2;
                if (τ < params.τ_min)
                    τ = 0;
                ++s.linesearch_backtracks;
                continue;
            }

            // QUB and line search satisfied
            break;
        }
        // If τ < τ_min the line search failed and we accepted the prox step
        s.linesearch_failures += (τ == 0 && τ_init > 0);
        s.τ_1_accepted += τ == 1;
        s.count_τ += 1;
        s.sum_τ += τ;

        // Check if we made any progress
        if (no_progress > 0 || k % params.max_no_progress == 0)
            no_progress = curr->x == next->x ? no_progress + 1 : 0;

        // Update L-BFGS -------------------------------------------------------

        if (τ_init < 1 || update_lbfgs_later) {
            if (curr->γ != next->γ) { // Flush L-BFGS if γ changed
                direction.changed_γ(next->γ, curr->γ);
                if (params.recompute_last_prox_step_after_lbfgs_flush) {
                    curr->γ = next->γ;
                    curr->L = next->L;
                    eval_prox_grad_step_in_prox(*curr);
                }
            }
            if (τ > 0 && params.update_direction_from_prox_step) {
                s.lbfgs_rejected += not direction.update(
                    curr->γ, next->γ, curr->x̂, next->x, prox->p, next->p,
                    prox->grad_ψ, next->grad_ψ);
            } else {
                s.lbfgs_rejected += not direction.update(
                    curr->γ, next->γ, curr->x, next->x, curr->p, next->p,
                    curr->grad_ψ, next->grad_ψ);
            }
        }

        // Advance step --------------------------------------------------------
        std::swap(curr, next);
        ++k;

#ifndef NDEBUG
        {
            ScopedMallocAllower ma;
            *prox = {n, m};
            *next = {n, m};
        }
#endif
    }
    throw std::logic_error("[ZeroFPR] loop error");
}

} // namespace alpaqa
