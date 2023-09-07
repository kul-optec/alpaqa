#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/functions/prox.hpp>
#include <cassert>
#include <cmath>
#include <stdexcept>

namespace alpaqa::functions {

/// ℓ₁-norm.
/// @ingroup grp_Functions
/// @tparam Weight
///         Type of weighting factors. Either scalar or vector.
template <Config Conf, class Weight = typename Conf::real_t>
    requires(std::is_same_v<Weight, typename Conf::real_t> ||
             std::is_same_v<Weight, typename Conf::vec> ||
             std::is_same_v<Weight, typename Conf::rvec> ||
             std::is_same_v<Weight, typename Conf::crvec>)
struct L1Norm {
    USING_ALPAQA_CONFIG(Conf);
    using weight_t                      = Weight;
    static constexpr bool scalar_weight = std::is_same_v<weight_t, real_t>;

    L1Norm(weight_t λ) : λ{std::move(λ)} {
        const char *msg = "L1Norm::λ must be nonnegative";
        if constexpr (scalar_weight) {
            if (λ < 0 || !std::isfinite(λ))
                throw std::invalid_argument(msg);
        } else {
            if ((λ.array() < 0).any() || !λ.allFinite())
                throw std::invalid_argument(msg);
        }
    }

    L1Norm()
        requires(scalar_weight)
        : λ{1} {}
    L1Norm()
        requires(!scalar_weight)
    = default;

    weight_t λ;

    real_t prox(crmat in, rmat out, real_t γ = 1) {
        assert(in.cols() == 1);
        assert(out.cols() == 1);
        assert(in.size() == out.size());
        const length_t n = in.size();
        if constexpr (scalar_weight) {
            assert(λ >= 0);
            if (λ == 0) {
                out = in;
                return 0;
            }
            auto step = vec::Constant(n, λ * γ);
            out       = vec::Zero(n).cwiseMax(in - step).cwiseMin(in + step);
            return λ * out.template lpNorm<1>();
        } else {
            if constexpr (std::is_same_v<weight_t, vec>)
                if (λ.size() == 0)
                    λ = weight_t::Ones(n);
            assert(λ.cols() == 1);
            assert(in.size() == λ.size());
            assert((λ.array() >= 0).all());
            auto step = λ * γ;
            out       = vec::Zero(n).cwiseMax(in - step).cwiseMin(in + step);
            return out.cwiseProduct(λ).template lpNorm<1>();
        }
    }

    friend real_t alpaqa_tag_invoke(tag_t<alpaqa::prox>, L1Norm &self, crmat in,
                                    rmat out, real_t γ) {
        return self.prox(std::move(in), std::move(out), γ);
    }
};

} // namespace alpaqa::functions
