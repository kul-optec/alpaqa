#pragma once

#include <alpaqa/outer/alm.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <utility>

#include <alpaqa/config/config.hpp>
#include <alpaqa/implementation/outer/internal/alm-helpers.tpp>
#include <alpaqa/implementation/util/print.tpp>
#include <alpaqa/inner/inner-solve-options.hpp>
#include <alpaqa/inner/internal/solverstatus.hpp>

namespace alpaqa {

template <class InnerSolverT>
typename ALMSolver<InnerSolverT>::Stats
ALMSolver<InnerSolverT>::operator()(const Problem &p, rvec x, rvec y,
                                    std::optional<rvec> Σ) {
    using std::chrono::duration_cast;
    using std::chrono::nanoseconds;
    auto start_time = std::chrono::steady_clock::now();

    // Check the problem dimensions etc.
    p.check();

    if (params.max_iter == 0)
        return {.status = SolverStatus::MaxIter};

    auto m = p.get_m();
    if (m == 0) { // No general constraints, only box constraints
        Stats s;
        vec Σ_curr(0), error(0);
        InnerSolveOptions<config_t> opts{
            .always_overwrite_results = true,
            .max_time                 = params.max_time,
            .tolerance                = params.tolerance,
            .os                       = os,
            .check                    = false,
        };
        auto ps              = inner_solver(p, opts, x, y, Σ_curr, error);
        bool inner_converged = ps.status == SolverStatus::Converged;
        auto time_elapsed    = std::chrono::steady_clock::now() - start_time;
        s.inner_convergence_failures = not inner_converged;
        s.inner += ps;
        s.ε                = ps.ε;
        s.δ                = 0;
        s.norm_penalty     = 0;
        s.outer_iterations = 1;
        s.elapsed_time     = duration_cast<nanoseconds>(time_elapsed);
        s.status           = ps.status;
        return s;
    }

    constexpr auto NaN = alpaqa::NaN<config_t>;
    vec Σ_curr         = vec::Constant(m, NaN);
    vec error          = vec::Constant(m, NaN);
    vec error_old      = vec::Constant(m, NaN);
    real_t norm_e = NaN, norm_e_old = NaN;

    std::array<char, 64> printbuf;
    auto print_real = [&](real_t x) {
        return float_to_str_vw(printbuf, x, params.print_precision);
    };

    Stats s;

    if (Σ && Σ->allFinite() && Σ->norm() > 0) {
        Σ_curr = *Σ;
    }
    // Initialize the penalty weights
    else if (params.initial_penalty > 0) {
        Σ_curr.fill(params.initial_penalty);
    }
    // Initial penalty weights from problem
    else {
        Helpers::initialize_penalty(p, params, x, Σ_curr);
    }

    // Inner solver tolerance
    real_t ε = params.initial_tolerance;

    for (unsigned i = 0; i < params.max_iter; ++i) {
        p.eval_proj_multipliers(y, params.max_multiplier);
        bool out_of_iter = i + 1 == params.max_iter;

        auto time_elapsed   = std::chrono::steady_clock::now() - start_time;
        auto time_remaining = time_elapsed < params.max_time
                                  ? params.max_time - time_elapsed
                                  : decltype(time_elapsed){0};
        InnerSolveOptions<config_t> opts{
            .always_overwrite_results = true,
            .max_time                 = time_remaining,
            .tolerance                = ε,
            .os                       = os,
            .outer_iter               = i,
            .check                    = false,
        };
        // Call the inner solver to minimize the augmented lagrangian for fixed
        // Lagrange multipliers y.
        auto ps = inner_solver(p, opts, x, y, Σ_curr, error);
        // Check if the inner solver converged
        bool inner_converged = ps.status == SolverStatus::Converged;
        // Accumulate the inner solver statistics
        s.inner_convergence_failures += not inner_converged;
        s.inner += ps;
        // Compute the constraint violation
        norm_e = vec_util::norm_inf(error);

        time_elapsed     = std::chrono::steady_clock::now() - start_time;
        bool out_of_time = time_elapsed > params.max_time;

        // Print statistics of current iteration
        if (params.print_interval != 0 && i % params.print_interval == 0) {
            const char *color = inner_converged ? "\x1b[0;32m" : "\x1b[0;31m";
            const char *color_end = "\x1b[0m";
            *os << "[\x1b[0;34mALM\x1b[0m]   " << std::setw(5) << i
                << ": ‖Σ‖ = " << print_real(Σ_curr.norm())
                << ", ‖y‖ = " << print_real(y.norm())
                << ", δ = " << print_real(norm_e)
                << ", ε = " << print_real(ps.ε) << ", status = " << color
                << std::setw(13) << ps.status << color_end
                << ", iter = " << std::setw(13) << ps.iterations
                << std::endl; // Flush for Python buffering
        }

        // TODO: check penalty size?
        if (ps.status == SolverStatus::Interrupted) {
            s.ε                = ps.ε;
            s.δ                = norm_e;
            s.norm_penalty     = Σ_curr.norm() / std::sqrt(real_t(m));
            s.outer_iterations = i + 1;
            s.elapsed_time     = duration_cast<nanoseconds>(time_elapsed);
            s.status           = ps.status;
            if (Σ)
                *Σ = Σ_curr;
            return s;
        }

        // Check the termination criteria
        bool alm_converged = ps.ε <= params.tolerance && inner_converged &&
                             norm_e <= params.dual_tolerance;
        bool exit = alm_converged || out_of_iter || out_of_time;
        if (exit) {
            s.ε                = ps.ε;
            s.δ                = norm_e;
            s.norm_penalty     = Σ_curr.norm() / std::sqrt(real_t(m));
            s.outer_iterations = i + 1;
            s.elapsed_time     = duration_cast<nanoseconds>(time_elapsed);
            s.status           = alm_converged ? SolverStatus::Converged
                                 : out_of_time ? SolverStatus::MaxTime
                                 : out_of_iter ? SolverStatus::MaxIter
                                               : SolverStatus::Busy;
            if (Σ)
                *Σ = Σ_curr;
            return s;
        }
        // Update Σ to contain the penalty to use on the next iteration.
        Helpers::update_penalty_weights(params, params.penalty_update_factor,
                                        i == 0, error, error_old, norm_e,
                                        norm_e_old, Σ_curr);
        // Lower the primal tolerance for the inner solver.
        ε = std::fmax(params.tolerance_update_factor * ε, params.tolerance);
        // Save previous error
        norm_e_old = norm_e;
        error.swap(error_old);
    }
    throw std::logic_error("[ALM]   loop error");
}

} // namespace alpaqa