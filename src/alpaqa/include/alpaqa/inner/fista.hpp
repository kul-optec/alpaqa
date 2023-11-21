#pragma once

#include <alpaqa/export.hpp>
#include <alpaqa/inner/directions/panoc-direction-update.hpp>
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

/// Tuning parameters for the FISTA algorithm.
/// @ingroup grp_Parameters
template <Config Conf = DefaultConfig>
struct FISTAParams {
    USING_ALPAQA_CONFIG(Conf);

    /// Parameters related to the Lipschitz constant estimate and step size.
    LipschitzEstimateParams<config_t> Lipschitz{};
    /// Maximum number of inner FISTA iterations.
    unsigned max_iter = 1000;
    /// Maximum duration.
    std::chrono::nanoseconds max_time = std::chrono::minutes(5);
    /// Minimum Lipschitz constant estimate.
    real_t L_min = real_t(1e-5);
    /// Maximum Lipschitz constant estimate.
    real_t L_max = real_t(1e20);
    /// What stopping criterion to use.
    PANOCStopCrit stop_crit = PANOCStopCrit::ApproxKKT;
    /// Maximum number of iterations without any progress before giving up.
    unsigned max_no_progress = 10;

    /// When to print progress. If set to zero, nothing will be printed.
    /// If set to N != 0, progress is printed every N iterations.
    unsigned print_interval = 0;
    /// The precision of the floating point values printed by the solver.
    int print_precision = std::numeric_limits<real_t>::max_digits10 / 2;

    /// Tolerance factor used in the quadratic upper bound condition that
    /// determines the step size. Its goal is to account for numerical errors
    /// in the function and gradient evaluations. If you notice that the step
    /// size γ becomes very small, you may want to increase this factor.
    real_t quadratic_upperbound_tolerance_factor =
        10 * std::numeric_limits<real_t>::epsilon();

    /// Don't compute accelerated steps, fall back to forward-backward splitting.
    /// For testing purposes.
    bool disable_acceleration = false;
};

template <Config Conf = DefaultConfig>
struct FISTAStats {
    USING_ALPAQA_CONFIG(Conf);

    SolverStatus status = SolverStatus::Busy;
    real_t ε            = inf<config_t>;
    std::chrono::nanoseconds elapsed_time{};
    std::chrono::nanoseconds time_progress_callback{};
    unsigned iterations          = 0;
    unsigned stepsize_backtracks = 0;
    real_t final_γ               = 0;
    real_t final_ψ               = 0;
    real_t final_h               = 0;
};

template <Config Conf = DefaultConfig>
struct FISTAProgressInfo {
    USING_ALPAQA_CONFIG(Conf);

    unsigned k;
    SolverStatus status;
    crvec x;
    crvec p;
    real_t norm_sq_p;
    crvec x̂;
    crvec ŷ;
    real_t φγ;
    real_t ψ;
    crvec grad_ψ;
    real_t ψ_hat;
    crvec grad_ψ_hat;
    real_t L;
    real_t γ;
    real_t t;
    real_t ε;
    crvec Σ;
    crvec y;
    unsigned outer_iter;
    const TypeErasedProblem<config_t> *problem;
    const FISTAParams<config_t> *params;
};

/// FISTA solver for ALM.
/// @ingroup    grp_InnerSolvers
template <Config Conf>
class FISTASolver {
  public:
    USING_ALPAQA_CONFIG(Conf);

    using Problem      = TypeErasedProblem<config_t>;
    using Params       = FISTAParams<config_t>;
    using Stats        = FISTAStats<config_t>;
    using ProgressInfo = FISTAProgressInfo<config_t>;
    using SolveOptions = InnerSolveOptions<config_t>;

    FISTASolver(const Params &params) : params(params) {}

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
    FISTASolver &
    set_progress_callback(std::function<void(const ProgressInfo &)> cb) {
        this->progress_cb = cb;
        return *this;
    }

    std::string get_name() const;

    void stop() { stop_signal.stop(); }

    const Params &get_params() const { return params; }

  private:
    Params params;
    AtomicStopSignal stop_signal;
    std::function<void(const ProgressInfo &)> progress_cb;
    using Helpers = detail::PANOCHelpers<config_t>;

  public:
    std::ostream *os = &std::cout;
};

template <class InnerSolverStats>
struct InnerStatsAccumulator;

template <Config Conf>
struct InnerStatsAccumulator<FISTAStats<Conf>> {
    USING_ALPAQA_CONFIG(Conf);

    /// Total elapsed time in the inner solver.
    std::chrono::nanoseconds elapsed_time{};
    /// Total time spent in the user-provided progress callback.
    std::chrono::nanoseconds time_progress_callback{};
    /// Total number of inner FISTA iterations.
    unsigned iterations = 0;
    /// Total number of FISTA step size reductions.
    unsigned stepsize_backtracks = 0;
    /// The final FISTA step size γ.
    real_t final_γ = 0;
    /// Final value of the smooth cost @f$ \psi(\hat x) @f$.
    real_t final_ψ = 0;
    /// Final value of the nonsmooth cost @f$ h(\hat x) @f$.
    real_t final_h = 0;
};

template <Config Conf>
InnerStatsAccumulator<FISTAStats<Conf>> &
operator+=(InnerStatsAccumulator<FISTAStats<Conf>> &acc,
           const FISTAStats<Conf> &s) {
    acc.iterations += s.iterations;
    acc.elapsed_time += s.elapsed_time;
    acc.time_progress_callback += s.time_progress_callback;
    acc.stepsize_backtracks += s.stepsize_backtracks;
    acc.final_γ  = s.final_γ;
    acc.final_ψ  = s.final_ψ;
    acc.final_h  = s.final_h;
    return acc;
}

// clang-format off
ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAParams, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAParams, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAParams, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAParams, EigenConfigq);)

ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAStats, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAStats, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAStats, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAStats, EigenConfigq);)

ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAProgressInfo, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAProgressInfo, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAProgressInfo, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, FISTAProgressInfo, EigenConfigq);)

ALPAQA_EXPORT_EXTERN_TEMPLATE(class, FISTASolver, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, FISTASolver, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, FISTASolver, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, FISTASolver, EigenConfigq);)
// clang-format on

} // namespace alpaqa
