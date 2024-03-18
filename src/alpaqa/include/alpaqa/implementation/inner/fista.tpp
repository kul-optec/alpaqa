#pragma once

#include <alpaqa/inner/fista.hpp>

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <utility>

#include <alpaqa/config/config.hpp>
#include <alpaqa/implementation/inner/panoc-helpers.tpp>
#include <alpaqa/implementation/util/print.tpp>
#include <alpaqa/util/alloc-check.hpp>
#include <alpaqa/util/timed.hpp>

namespace alpaqa {

template <Config Conf>
std::string FISTASolver<Conf>::get_name() const {
    return "FISTASolver<" + std::string(config_t::get_name()) + ">";
}

/*
    Beck:

    x₀ = x̂₀ = guess
    t₀ = 1
    for k = 0, 1, 2, ...
        x̂ₖ = FB(xₖ; γₖ)
        tₖ₊₁ = (1 + √(1 + 4 tₖ²)) / 2
        xₖ₊₁ = x̂ₖ₊₁ + (tₖ-1)/tₖ₊₁ (x̂ₖ₊₁ - x̂ₖ)

    With line search:

    x₀ = x̂₀ = guess
    t₀ = 1
    for k = 0, 1, 2, ...
        eval ψ(xₖ), ∇ψ(xₖ)
        x̂ₖ = FB(xₖ; γₖ)
        eval ψ(x̂ₖ)
        if QUB violated
            γₖ /= 2
            x̂ₖ = FB(xₖ; γₖ)
            eval ψ(x̂ₖ)
        tₖ₊₁ = (1 + √(1 + 4 tₖ²)) / 2
        xₖ₊₁ = x̂ₖ₊₁ + (tₖ-1)/tₖ₊₁ (x̂ₖ₊₁ - x̂ₖ)

    Move gradient evaluation:

    x₀ = x̂₀ = guess
    t₀ = 1
    eval ψ(x₀), ∇ψ(x₀)
    for k = 0, 1, 2, ...
        x̂ₖ = FB(xₖ; γₖ)
        eval ψ(x̂ₖ)
        if QUB violated
            γₖ /= 2
            x̂ₖ = FB(xₖ; γₖ)
            eval ψ(x̂ₖ)
        tₖ₊₁ = (1 + √(1 + 4 tₖ²)) / 2
        xₖ₊₁ = x̂ₖ₊₁ + (tₖ-1)/tₖ₊₁ (x̂ₖ₊₁ - x̂ₖ)
        eval ψ(xₖ₊₁), ∇ψ(xₖ₊₁)

*/

template <Config Conf>
auto FISTASolver<Conf>::operator()(
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

    if (opts.check)
        problem.check();

    using std::chrono::nanoseconds;
    auto os         = opts.os ? opts.os : this->os;
    auto start_time = std::chrono::steady_clock::now();
    Stats s;

    const auto n = problem.get_n();
    const auto m = problem.get_m();

    // Represents an iterate in the algorithm, keeping track of some
    // intermediate values and function evaluations.
    struct Iterate {
        vec x;       //< Decision variables
        vec x̂;       //< Forward-backward point of x
        vec grad_ψ;  //< Gradient of cost in x
        vec grad_ψx̂; //< Gradient of cost in x̂
        vec p;       //< Proximal gradient step in x
        vec ŷx̂;      //< Candidate Lagrange multipliers in x̂
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
    } iterate{n, m};
    Iterate *curr = &iterate;

    bool need_grad_ψx̂ = Helpers::stop_crit_requires_grad_ψx̂(params.stop_crit);
    if (need_grad_ψx̂)
        curr->grad_ψx̂.resize(n);

    vec work_n1(n), work_n2(n), work_m(m);
    vec prev_x̂(n); // storage to remember x̂ₖ while computing x̂ₖ₊₁

    // Helper functions --------------------------------------------------------

    auto qub_violated = [this](const Iterate &i) {
        real_t margin =
            (1 + std::abs(i.ψx)) * params.quadratic_upperbound_tolerance_factor;
        return i.ψx̂ > i.ψx + i.grad_ψᵀp + real_t(0.5) * i.L * i.pᵀp + margin;
    };

    // Problem functions -------------------------------------------------------

    auto eval_ψ_grad_ψ = [&problem, &y, &Σ, &work_n1, &work_m](Iterate &i) {
        i.ψx = problem.eval_ψ_grad_ψ(i.x, y, Σ, i.grad_ψ, work_n1, work_m);
    };
    auto eval_grad_ψ = [&problem, &y, &Σ, &work_n1, &work_m](Iterate &i) {
        problem.eval_grad_ψ(i.x, y, Σ, i.grad_ψ, work_n1, work_m);
    };
    auto eval_prox_grad_step = [&problem](Iterate &i) {
        i.hx̂  = problem.eval_prox_grad_step(i.γ, i.x, i.grad_ψ, i.x̂, i.p);
        i.pᵀp = i.p.squaredNorm();
        i.grad_ψᵀp = i.p.dot(i.grad_ψ);
    };
    auto eval_ψx̂ = [&problem, &y, &Σ](Iterate &i) {
        i.ψx̂ = problem.eval_ψ(i.x̂, y, Σ, i.ŷx̂);
    };
    auto eval_grad_ψx̂ = [&problem, &work_n1](Iterate &i) {
        // assumes that eval_ψx̂ was called first
        problem.eval_grad_L(i.x̂, i.ŷx̂, i.grad_ψx̂, work_n1);
    };

    // Printing ----------------------------------------------------------------

    std::array<char, 64> print_buf;
    auto print_real = [this, &print_buf](real_t x) {
        return float_to_str_vw(print_buf, x, params.print_precision);
    };
    auto print_progress_1 = [&print_real, os](unsigned k, real_t ψₖ,
                                              crvec grad_ψₖ, real_t pₖᵀpₖ,
                                              real_t γₖ, real_t εₖ) {
        if (k == 0)
            *os << "┌─[FISTA]\n";
        else
            *os << "├─ " << std::setw(6) << k << '\n';
        *os << "|    ψ = " << print_real(ψₖ)               //
            << ", ‖∇ψ‖ = " << print_real(grad_ψₖ.norm())   //
            << ",  ‖p‖ = " << print_real(std::sqrt(pₖᵀpₖ)) //
            << ",    γ = " << print_real(γₖ)               //
            << ",    ε = " << print_real(εₖ) << '\n';
    };
    auto print_progress_n = [&](SolverStatus status) {
        *os << "└─ " << status << " ──"
            << std::endl; // Flush for Python buffering
    };

    auto do_progress_cb = [this, &s, &problem, &Σ, &y,
                           &opts](unsigned k, Iterate &it, real_t t, real_t εₖ,
                                  SolverStatus status) {
        if (!progress_cb)
            return;
        ScopedMallocAllower ma;
        alpaqa::util::Timed timed{s.time_progress_callback};
        progress_cb(ProgressInfo{
            .k          = k,
            .status     = status,
            .x          = it.x,
            .p          = it.p,
            .norm_sq_p  = it.pᵀp,
            .x̂          = it.x̂,
            .ŷ          = it.ŷx̂,
            .φγ         = it.fbe(),
            .ψ          = it.ψx,
            .grad_ψ     = it.grad_ψ,
            .ψ_hat      = it.ψx̂,
            .grad_ψ_hat = it.grad_ψx̂,
            .L          = it.L,
            .γ          = it.γ,
            .t          = t,
            .ε          = εₖ,
            .Σ          = Σ,
            .y          = y,
            .outer_iter = opts.outer_iter,
            .problem    = &problem,
            .params     = &params,
        });
    };

    // Initialization ----------------------------------------------------------

    curr->x = x;
    curr->x̂ = x;

    // Estimate Lipschitz constant ---------------------------------------------

    bool fixed_lipschitz = params.L_min == params.L_max;
    // Fixed Lipschitz constant provided by user, no backtracking
    if (fixed_lipschitz) {
        curr->L = params.L_max;
        // Calculate ∇ψ(x₀)
        eval_grad_ψ(*curr);
    }
    // Finite difference approximation of ∇²ψ in starting point
    else if (params.Lipschitz.L_0 <= 0) {
        curr->L = Helpers::initial_lipschitz_estimate(
            problem, curr->x, y, Σ, params.Lipschitz.ε, params.Lipschitz.δ,
            params.L_min, params.L_max,
            /* in ⟹ out */ curr->ψx, curr->grad_ψ,
            /* work */ curr->x̂, work_n1, work_n2, work_m);
    }
    // Initial Lipschitz constant provided by the user
    else {
        curr->L = params.Lipschitz.L_0;
        // Calculate ψ(x₀), ∇ψ(x₀)
        eval_ψ_grad_ψ(*curr);
    }
    if (not std::isfinite(curr->L)) {
        s.status = SolverStatus::NotFinite;
        return s;
    }
    curr->γ = params.Lipschitz.Lγ_factor / curr->L;
    // ψ(x₀), ∇ψ(x₀) are now available in curr

    // Loop data ---------------------------------------------------------------

    unsigned k = 0; // iteration
    real_t t   = 1; // acceleration parameter
    // Keep track of how many successive iterations didn't update the iterate
    unsigned no_progress = 0;

    // Main FISTA loop
    // =========================================================================

    ScopedMallocBlocker mb; // Don't allocate in the inner loop
    while (true) {
        // Proximal gradient step ----------------------------------------------

        prev_x̂.swap(curr->x̂); // Remember x̂ₖ
        eval_prox_grad_step(*curr);

        // Calculate ψ(x̂ₖ), ∇ψ(x̂ₖ), ŷ
        if (!fixed_lipschitz || need_grad_ψx̂)
            eval_ψx̂(*curr);
        if (need_grad_ψx̂)
            eval_grad_ψx̂(*curr);

        // Quadratic upper bound -----------------------------------------------

        while (curr->L < params.L_max && qub_violated(*curr)) {
            curr->γ /= 2;
            curr->L *= 2;
            eval_prox_grad_step(*curr);
            eval_ψx̂(*curr);
            ++s.stepsize_backtracks;
        }

        // Check stopping criteria ---------------------------------------------

        // Check if we made any progress
        if (no_progress > 0 || k % params.max_no_progress == 0)
            no_progress = curr->x̂ == prev_x̂ ? no_progress + 1 : 0;

        real_t εₖ = Helpers::calc_error_stop_crit(
            problem, params.stop_crit, curr->p, curr->γ, curr->x, curr->x̂,
            curr->ŷx̂, curr->grad_ψ, curr->grad_ψx̂, work_n1, work_n2);

        auto time_elapsed = std::chrono::steady_clock::now() - start_time;
        auto stop_status  = Helpers::check_all_stop_conditions(
            params, opts, time_elapsed, k, stop_signal, εₖ, no_progress);

        // Return solution -----------------------------------------------------

        if (stop_status != SolverStatus::Busy) {
            do_progress_cb(k, *curr, t, εₖ, stop_status);
            if (params.print_interval) {
                print_progress_1(k, curr->ψx, curr->grad_ψ, curr->pᵀp, curr->γ,
                                 εₖ);
                print_progress_n(stop_status);
            }
            // Calculate ψ(x̂ₖ), ŷ
            if (fixed_lipschitz && !need_grad_ψx̂)
                eval_ψx̂(*curr);
            if (stop_status == SolverStatus::Converged ||
                stop_status == SolverStatus::Interrupted ||
                opts.always_overwrite_results) {
                auto &ŷ = curr->ŷx̂;
                if (err_z.size() > 0)
                    err_z = (ŷ - y).cwiseQuotient(Σ);
                x = curr->x̂;
                y = curr->ŷx̂;
            }
            s.iterations   = k;
            s.ε            = εₖ;
            s.elapsed_time = duration_cast<nanoseconds>(time_elapsed);
            s.status       = stop_status;
            s.final_γ      = curr->γ;
            s.final_ψ      = curr->ψx̂;
            s.final_h      = curr->hx̂;
            return s;
        }

        // Print progress ------------------------------------------------------

        bool do_print =
            params.print_interval != 0 && k % params.print_interval == 0;
        if (do_print)
            print_progress_1(k, curr->ψx, curr->grad_ψ, curr->pᵀp, curr->γ, εₖ);

        // Progress callback ---------------------------------------------------

        do_progress_cb(k, *curr, t, εₖ, SolverStatus::Busy);

        // Calculate next point ------------------------------------------------

        // Calculate tₖ₊₁
        real_t t_new  = (1 + std::sqrt(1 + 4 * t)) / 2;
        real_t t_prev = std::exchange(t, t_new);
        // Calculate xₖ₊₁
        if (params.disable_acceleration)
            curr->x = curr->x̂;
        else
            curr->x = curr->x̂ + ((t_prev - 1) / t) * (curr->x̂ - prev_x̂);
        // Calculate ψ(xₖ), ∇ψ(xₖ)
        if (fixed_lipschitz)
            eval_grad_ψ(*curr);
        else
            eval_ψ_grad_ψ(*curr);

        // Advance step --------------------------------------------------------
        ++k;
    }
    throw std::logic_error("[FISTA] loop error");
}

} // namespace alpaqa
