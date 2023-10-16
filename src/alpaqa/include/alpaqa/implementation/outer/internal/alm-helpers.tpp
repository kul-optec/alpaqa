#pragma once

#include <alpaqa/outer/alm.hpp>
#if ALPAQA_WITH_OCP
#include <alpaqa/problem/ocproblem.hpp>
#endif

#include <algorithm>

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
            return;
        }
        if (params.single_penalty_factor) {
            if (first_iter || norm_e > θ * old_norm_e) {
                real_t new_Σ = std::fmin(params.max_penalty, Δ * Σ(0));
                Σ.setConstant(new_Σ);
            }
        } else {
            for (index_t i = 0; i < e.rows(); ++i) {
                if (first_iter || std::abs(e(i)) > θ * std::abs(old_e(i))) {
                    Σ(i) = std::fmin(
                        params.max_penalty,
                        std::fmax(Δ * std::abs(e(i)) / norm_e, real_t(1)) *
                            Σ(i));
                }
            }
        }
    }

    static void initialize_penalty(const TypeErasedProblem<config_t> &p,
                                   const ALMParams<config_t> &params, crvec x0,
                                   rvec Σ) {
        real_t f0 = p.eval_f(x0);
        vec g0(p.get_m());
        p.eval_g(x0, g0);
        // TODO: reuse evaluations of f ang g in PANOC?
        real_t σ = params.initial_penalty_factor *
                   std::max(real_t(1), std::abs(f0)) /
                   std::max(real_t(1), real_t(0.5) * g0.squaredNorm());
        σ = std::clamp(σ, params.min_penalty, params.max_penalty);
        Σ.fill(σ);
    }

#if ALPAQA_WITH_OCP
    static void initialize_penalty(
        [[maybe_unused]] const TypeErasedControlProblem<config_t> &p,
        const ALMParams<config_t> &params, [[maybe_unused]] crvec x0, rvec Σ) {
        real_t σ = 1;
        σ        = std::clamp(σ, params.min_penalty, params.max_penalty);
        Σ.fill(σ);
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