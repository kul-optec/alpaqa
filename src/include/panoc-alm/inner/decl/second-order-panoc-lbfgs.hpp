#pragma once

#include <panoc-alm/inner/decl/panoc-fwd.hpp>
#include <panoc-alm/inner/directions/decl/lbfgs.hpp>
#include <panoc-alm/util/atomic_stop_signal.hpp>
#include <panoc-alm/util/problem.hpp>
#include <panoc-alm/util/solverstatus.hpp>

#include <atomic>
#include <chrono>
#include <limits>
#include <string>

namespace pa {

/// Tuning parameters for the second order PANOC algorithm.
struct SecondOrderPANOCLBFGSParams {
    struct {
        /// Relative step size for initial finite difference Lipschitz estimate.
        real_t ε = 1e-6;
        /// Minimum step size for initial finite difference Lipschitz estimate.
        real_t δ = 1e-12;
        /// Factor that relates step size γ and Lipschitz constant.
        real_t Lγ_factor = 0.95;
    } Lipschitz; ///< Parameters related to the Lipschitz constant estimate
                 ///  and step size.

    /// Length of the history to keep in the L-BFGS algorithm.
    unsigned lbfgs_mem = 10; // TODO: move to LBFGS params
    /// Maximum number of inner PANOC iterations.
    unsigned max_iter = 100;
    /// Maximum duration.
    std::chrono::microseconds max_time = std::chrono::minutes(5);
    /// Minimum weight factor between Newton step and projected gradient step.
    real_t τ_min = 1. / 256;
    /// Maximum number of iterations without any progress before giving up.
    unsigned max_no_progress = 10;

    /// When to print progress. If set to zero, nothing will be printed.
    /// If set to N != 0, progress is printed every N iterations.
    unsigned print_interval = 0;

    real_t quadratic_upperbound_tolerance_factor =
        10 * std::numeric_limits<real_t>::epsilon();

    bool update_lipschitz_in_linesearch = true;
    bool alternative_linesearch_cond    = false;

    enum {
        BasedOnGradientStepSize = 0,
        BasedOnCurvature        = 1,
    } lbfgs_stepsize = BasedOnCurvature;
};

/// Second order PANOC solver for ALM.
/// @ingroup    grp_InnerSolvers
class SecondOrderPANOCLBFGSSolver {
  public:
    using Params = SecondOrderPANOCLBFGSParams;

    struct Stats {
        unsigned iterations = 0;
        real_t ε            = inf;
        std::chrono::microseconds elapsed_time;
        SolverStatus status          = SolverStatus::Unknown;
        unsigned linesearch_failures = 0;

        unsigned lbfgs_failures = 0; // TODO: remove
        unsigned lbfgs_rejected = 0; // TODO: remove
    };

    struct ProgressInfo {
        unsigned k;
        const vec &x;
        const vec &p;
        real_t norm_sq_p;
        const vec &x_hat;
        real_t ψ;
        const vec &grad_ψ;
        real_t ψ_hat;
        const vec &grad_ψ_hat;
        real_t L;
        real_t γ;
        real_t ε;
        const vec &Σ;
        const vec &y;
        const Problem &problem;
        const Params &params;
    };

    SecondOrderPANOCLBFGSSolver(Params params, LBFGSParams lbfgsparams)
        : params(params), lbfgs(lbfgsparams) {}

    Stats operator()(const Problem &problem,        // in
                     const vec &Σ,                  // in
                     real_t ε,                      // in
                     bool always_overwrite_results, // in
                     vec &x,                        // inout
                     vec &y,                        // inout
                     vec &err_z);                   // out

    SecondOrderPANOCLBFGSSolver &
    set_progress_callback(std::function<void(const ProgressInfo &)> cb) {
        this->progress_cb = cb;
        return *this;
    }

    std::string get_name() const { return "SecondOrderPANOCSolverLBFGS"; }

    void stop() { stop_signal.stop(); }

    const Params &get_params() const { return params; }

  private:
    Params params;
    AtomicStopSignal stop_signal;
    std::function<void(const ProgressInfo &)> progress_cb;

  public:
    LBFGS lbfgs;
};

} // namespace pa