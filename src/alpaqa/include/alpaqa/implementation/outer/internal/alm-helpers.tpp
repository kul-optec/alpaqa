#pragma once

#include <alpaqa/outer/alm.hpp>
#if ALPAQA_WITH_OCP
#include <alpaqa/problem/ocproblem.hpp>
#endif

#include <algorithm>
#include <stdexcept>

namespace alpaqa::detail {

template <Config Conf>
struct ALMHelpers {
    USING_ALPAQA_CONFIG(Conf);

    static void update_penalty_weights(const ALMParams<config_t> &params,
                                       real_t Δ, bool first_iter, rvec e,
                                       rvec old_e, real_t norm_e,
                                       real_t old_norm_e, rvec Σ) {
        const real_t θ = params.rel_penalty_increase_threshold;
        if (norm_e <= params.dual_tolerance) {
            // Don't update the penalty factors if the constraint violation is
            // already below the required tolerance.
            return;
        }
        if (params.single_penalty_factor) {
            if constexpr (requires { Σ(0); }) {
                if (first_iter || norm_e > θ * old_norm_e) {
                    real_t new_Σ = std::fmin(params.max_penalty, Δ * Σ(0));
                    Σ.setConstant(new_Σ);
                }
            } else {
                throw std::logic_error("This configuration does not support "
                                       "single-penalty parameter mode");
            }
        } else {
            auto new_Σ = (e.cwiseAbs() * (Δ / norm_e))
                             .cwiseMax(1)
                             .cwiseProduct(Σ)
                             .cwiseMin(params.max_penalty);
            if (first_iter) {
                // Update the penalty factors regardless of previous error
                // (because we don't have the previous error yet).
                // TODO: we could in theory evaluate it before the first
                // iteration, the inner solver computes it anyway, but this may
                // add unnecessary complexity.
                Σ = new_Σ;
            } else {
                // Decide which constraints' penalty factors to increase.
                auto incr = e.cwiseAbs().array() > θ * old_e.cwiseAbs().array();
                Σ = incr.select(new_Σ, Σ);
            }
        }
    }

    static void initialize_penalty(const TypeErasedProblem<config_t> &p,
                                   const ALMParams<config_t> &params, crvec x0,
                                   rvec Σ) {
        real_t f0 = p.eval_objective(x0);
        vec g0(p.get_num_constraints());
        p.eval_constraints(x0, g0);
        // TODO: reuse evaluations of f ang g in PANOC?
        real_t σ = params.initial_penalty_factor *
                   std::max(real_t(1), std::abs(f0)) /
                   std::max(real_t(1), real_t(0.5) * g0.squaredNorm());
        σ = std::clamp(σ, params.min_penalty, params.max_penalty);
        Σ.setConstant(σ);
    }

#if ALPAQA_WITH_OCP
    static void initialize_penalty(
        [[maybe_unused]] const TypeErasedControlProblem<config_t> &p,
        const ALMParams<config_t> &params, [[maybe_unused]] crvec x0, rvec Σ) {
        real_t σ = 1;
        σ        = std::clamp(σ, params.min_penalty, params.max_penalty);
        Σ.setConstant(σ);
    }
#endif
};

// clang-format off
ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, ALMHelpers, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, ALMHelpers, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, ALMHelpers, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, ALMHelpers, EigenConfigq);)
// clang-format on

} // namespace alpaqa::detail