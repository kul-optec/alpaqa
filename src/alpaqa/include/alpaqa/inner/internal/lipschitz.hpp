#pragma once

#include <alpaqa/config/config.hpp>

namespace alpaqa {

/// Parameters for the estimation of the Lipschitz constant of the gradient of
/// the smooth term of the cost. This is needed to select a suitable step size
/// for the forward-backward iterations used by solvers like PANOC and PANTR.
/// @ingroup grp_Parameters
template <Config Conf = DefaultConfig>
struct LipschitzEstimateParams {
    USING_ALPAQA_CONFIG(Conf);

    /// Initial estimate of the Lipschitz constant of ∇ψ(x). If set to zero,
    /// it will be approximated using finite differences.
    real_t L_0 = 0;
    /// Relative step size for initial finite difference Lipschitz estimate.
    real_t ε = real_t(1e-6);
    /// Minimum step size for initial finite difference Lipschitz estimate.
    real_t δ = real_t(1e-12);
    /// Factor that relates step size γ and Lipschitz constant.
    /// Parameter α in Algorithm 2 of @cite de_marchi_proximal_2022.
    /// @f$ 0 < \alpha < 1 @f$
    real_t Lγ_factor = real_t(0.95);
};

} // namespace alpaqa