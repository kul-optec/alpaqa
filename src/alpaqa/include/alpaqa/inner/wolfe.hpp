#pragma once

#include <alpaqa/export.hpp>
#include <alpaqa/inner/inner-solve-options.hpp>
#include <alpaqa/inner/internal/lipschitz.hpp>
#include <alpaqa/inner/internal/panoc-helpers.hpp>
#include <alpaqa/inner/internal/panoc-stop-crit.hpp>
#include <alpaqa/inner/internal/solverstatus.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>
#include <alpaqa/util/atomic-stop-signal.hpp>

#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace alpaqa {

/// Tuning parameters for the unconstrained solver with Wolfe line search.
/// @ingroup grp_Parameters
template <Config Conf = DefaultConfig>
struct WolfeParams {
    USING_ALPAQA_CONFIG(Conf);

    /// Maximum number of inner iterations.
    unsigned max_iter = 100;
    /// Maximum duration.
    std::chrono::nanoseconds max_time = std::chrono::minutes(5);
    /// Minimum weight factor between Newton step and projected gradient step.
    real_t min_linesearch_coefficient = real_t(1. / 256);
    /// Ignore the line search condition and always accept the accelerated step.
    /// (For testing purposes only).
    bool force_linesearch   = false;
    real_t decrease_factor  = real_t(1e-4);
    real_t curvature_factor = NaN<config_t>;
    /// What stopping criterion to use.
    PANOCStopCrit stop_crit = PANOCStopCrit::ApproxKKT;
    /// Maximum number of iterations without any progress before giving up.
    unsigned max_no_progress = 10;

    /// When to print progress. If set to zero, nothing will be printed.
    /// If set to N != 0, progress is printed every N iterations.
    unsigned print_interval = 0;
    /// The precision of the floating point values printed by the solver.
    int print_precision = std::numeric_limits<real_t>::max_digits10 / 2;

    /// Tolerance factor used in the line search. Its goal is to account for
    /// numerical errors in the function and gradient evaluations. If you notice
    /// that accelerated steps are rejected (τ = 0) when getting closer to the
    /// solution, you may want to increase this factor.
    real_t linesearch_tolerance_factor =
        10 * std::numeric_limits<real_t>::epsilon();
};

/// Statistics for the unconstrained solver with Wolfe line search.
template <Config Conf = DefaultConfig>
struct WolfeStats {
    USING_ALPAQA_CONFIG(Conf);

    SolverStatus status = SolverStatus::Busy;
    real_t ε            = inf<config_t>;
    std::chrono::nanoseconds elapsed_time{};
    std::chrono::nanoseconds time_progress_callback{};
    unsigned iterations            = 0;
    unsigned linesearch_failures   = 0;
    unsigned linesearch_backtracks = 0;
    unsigned lbfgs_failures        = 0;
    unsigned lbfgs_rejected        = 0;
    unsigned τ_1_accepted          = 0;
    unsigned count_τ               = 0;
    real_t sum_τ                   = 0;
    real_t final_ψ                 = 0;
};

/// Iterate information for the unconstrained solver with Wolfe line search.
template <Config Conf = DefaultConfig>
struct WolfeProgressInfo {
    USING_ALPAQA_CONFIG(Conf);

    unsigned k;
    SolverStatus status;
    crvec x;
    real_t ψ;
    crvec grad_ψ;
    crvec q;
    real_t τ;
    real_t ε;
    crvec Σ;
    crvec y;
    unsigned outer_iter;
    const TypeErasedProblem<config_t> *problem;
    const WolfeParams<config_t> *params;
};

/// Unconstrained solver with Wolfe line search.
/// @ingroup    grp_InnerSolvers
template <class DirectionT>
class WolfeSolver {
  public:
    USING_ALPAQA_CONFIG_TEMPLATE(DirectionT::config_t);

    using Problem      = TypeErasedProblem<config_t>;
    using Params       = WolfeParams<config_t>;
    using Direction    = DirectionT;
    using Stats        = WolfeStats<config_t>;
    using ProgressInfo = WolfeProgressInfo<config_t>;
    using SolveOptions = InnerSolveOptions<config_t>;

    WolfeSolver(const Params &params)
        requires std::default_initializable<Direction>
        : params(params) {}
    WolfeSolver(const Params &params, Direction &&direction)
        : params(params), direction(std::move(direction)) {}
    WolfeSolver(const Params &params, const Direction &direction)
        : params(params), direction(direction) {}

    Stats operator()(const Problem &problem,   // in
                     const SolveOptions &opts, // in
                     rvec x,                   // inout
                     rvec y,                   // inout
                     crvec Σ,                  // in
                     rvec err_z);              // out

    template <class P>
    Stats operator()(const P &problem, const SolveOptions &opts, rvec x, rvec y,
                     crvec Σ, rvec e) {
        return operator()(Problem{&problem}, opts, x, y, Σ, e);
    }

    template <class P>
    Stats operator()(const P &problem, const SolveOptions &opts, rvec x) {
        if (problem.get_m() != 0)
            throw std::invalid_argument("Missing arguments y, Σ, e");
        mvec y{nullptr, 0}, Σ{nullptr, 0}, e{nullptr, 0};
        return operator()(problem, opts, x, y, Σ, e);
    }

    /// Specify a callable that is invoked with some intermediate results on
    /// each iteration of the algorithm.
    /// @see @ref ProgressInfo
    WolfeSolver &
    set_progress_callback(std::function<void(const ProgressInfo &)> cb) {
        this->progress_cb = cb;
        return *this;
    }

    [[nodiscard]] std::string get_name() const;

    void stop() { stop_signal.stop(); }

    const Params &get_params() const { return params; }

  private:
    Params params;
    AtomicStopSignal stop_signal;
    std::function<void(const ProgressInfo &)> progress_cb;
    using Helpers = detail::PANOCHelpers<config_t>;

  public:
    Direction direction;
    std::ostream *os = &std::cout;
};

template <class InnerSolverStats>
struct InnerStatsAccumulator;

template <Config Conf>
struct InnerStatsAccumulator<WolfeStats<Conf>> {
    USING_ALPAQA_CONFIG(Conf);

    /// Total elapsed time in the inner solver.
    std::chrono::nanoseconds elapsed_time{};
    /// Total time spent in the user-provided progress callback.
    std::chrono::nanoseconds time_progress_callback{};
    /// Total number of inner Wolfe iterations.
    unsigned iterations = 0;
    /// Total number of Wolfe line search failures.
    unsigned linesearch_failures = 0;
    /// Total number of Wolfe line search backtracking steps.
    unsigned linesearch_backtracks = 0;
    /// Total number of times that the L-BFGS direction was not finite.
    unsigned lbfgs_failures = 0;
    /// Total number of times that the L-BFGS update was rejected (i.e. it
    /// could have resulted in a non-positive definite Hessian estimate).
    unsigned lbfgs_rejected = 0;
    /// Total number of times that a line search parameter of @f$ \tau = 1 @f$
    /// was accepted (i.e. no backtracking necessary).
    unsigned τ_1_accepted = 0;
    /// The total number of line searches performed (used for computing the
    /// average value of @f$ \tau @f$).
    unsigned count_τ = 0;
    /// The sum of the line search parameter @f$ \tau @f$ in all iterations
    /// (used for computing the average value of @f$ \tau @f$).
    real_t sum_τ = 0;
    /// Final value of the smooth cost @f$ \psi(\hat x) @f$.
    real_t final_ψ = 0;
};

template <Config Conf>
InnerStatsAccumulator<WolfeStats<Conf>> &
operator+=(InnerStatsAccumulator<WolfeStats<Conf>> &acc,
           const WolfeStats<Conf> &s) {
    acc.iterations += s.iterations;
    acc.elapsed_time += s.elapsed_time;
    acc.time_progress_callback += s.time_progress_callback;
    acc.linesearch_failures += s.linesearch_failures;
    acc.linesearch_backtracks += s.linesearch_backtracks;
    acc.lbfgs_failures += s.lbfgs_failures;
    acc.lbfgs_rejected += s.lbfgs_rejected;
    acc.τ_1_accepted += s.τ_1_accepted;
    acc.count_τ += s.count_τ;
    acc.sum_τ += s.sum_τ;
    acc.final_ψ = s.final_ψ;
    return acc;
}

// clang-format off
ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeParams, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeParams, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeParams, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeParams, EigenConfigq);)

ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeStats, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeStats, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeStats, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeStats, EigenConfigq);)

ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeProgressInfo, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeProgressInfo, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeProgressInfo, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, WolfeProgressInfo, EigenConfigq);)
// clang-format on

} // namespace alpaqa
