#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/functions/prox.hpp>
#include <alpaqa/util/lifetime.hpp>
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
        using vec_util::norm_1;
        const length_t n = in.size();
        if constexpr (scalar_weight) {
            assert(λ >= 0);
            if (λ == 0) {
                out = in;
                return 0;
            }
            auto step = vec::Constant(n, λ * γ);
            out       = vec::Zero(n).cwiseMax(in - step).cwiseMin(in + step);
            return λ * norm_1(out.reshaped());
        } else {
            if constexpr (std::is_same_v<weight_t, vec>)
                if (λ.size() == 0)
                    λ = weight_t::Ones(n);
            assert(λ.cols() == 1);
            assert(in.size() == λ.size());
            assert((λ.array() >= 0).all());
            auto step = λ * γ;
            out       = vec::Zero(n).cwiseMax(in - step).cwiseMin(in + step);
            return norm_1(out.cwiseProduct(λ).reshaped());
        }
    }

    friend real_t alpaqa_tag_invoke(tag_t<alpaqa::prox>, L1Norm &self, crmat in,
                                    rmat out, real_t γ) {
        return self.prox(std::move(in), std::move(out), γ);
    }
};

/// ℓ₁-norm of complex numbers.
/// @ingroup grp_Functions
/// @tparam Weight
///         Type of weighting factors. Either scalar or vector.
template <Config Conf, class Weight = typename Conf::real_t>
    requires(std::is_same_v<Weight, typename Conf::real_t> ||
             std::is_same_v<Weight, typename Conf::vec> ||
             std::is_same_v<Weight, typename Conf::rvec> ||
             std::is_same_v<Weight, typename Conf::crvec>)
struct L1NormComplex {
    USING_ALPAQA_CONFIG(Conf);
    using weight_t                      = Weight;
    static constexpr bool scalar_weight = std::is_same_v<weight_t, real_t>;

    L1NormComplex(weight_t λ) : λ{std::move(λ)} {
        const char *msg = "L1NormComplex::λ must be nonnegative";
        if constexpr (scalar_weight) {
            if (λ < 0 || !std::isfinite(λ))
                throw std::invalid_argument(msg);
        } else {
            if ((λ.array() < 0).any() || !λ.allFinite())
                throw std::invalid_argument(msg);
        }
    }

    L1NormComplex()
        requires(scalar_weight)
        : λ{1} {}
    L1NormComplex()
        requires(!scalar_weight)
    = default;

    weight_t λ;

    real_t prox(crcmat in, rcmat out, real_t γ = 1) {
        assert(in.cols() == 1);
        assert(out.cols() == 1);
        assert(in.size() == out.size());
        using vec_util::norm_1;
        const length_t n = in.size();
        if constexpr (scalar_weight) {
            assert(λ >= 0);
            if (λ == 0) {
                out = in;
                return 0;
            }
            auto soft_thres = [γλ{γ * λ}](cplx_t x) {
                auto mag2 = x.real() * x.real() + x.imag() * x.imag();
                return mag2 <= γλ * γλ ? 0 : x * (1 - γλ / std::sqrt(mag2));
            };
            out = in.unaryExpr(soft_thres);
            return λ * norm_1(out);
        } else {
            if constexpr (std::is_same_v<weight_t, vec>)
                if (λ.size() == 0)
                    λ = weight_t::Ones(n);
            assert(λ.cols() == 1);
            assert(in.size() == λ.size());
            assert((λ.array() >= 0).all());
            auto soft_thres = [γ](cplx_t x, real_t λ) {
                real_t γλ = γ * λ;
                auto mag2 = x.real() * x.real() + x.imag() * x.imag();
                return mag2 <= γλ * γλ ? 0 : x * (1 - γλ / std::sqrt(mag2));
            };
            out = in.binaryExpr(λ, soft_thres);
            return norm_1(out.cwiseProduct(λ));
        }
    }

    /// Note: a complex vector in ℂⁿ is represented by a real vector in ℝ²ⁿ.
    real_t prox(crmat in, rmat out, real_t γ = 1) {
        assert(in.rows() % 2 == 0);
        assert(out.rows() % 2 == 0);
        cmcmat cplx_in{
            util::start_lifetime_as_array<cplx_t>(in.data(), in.size() / 2),
            in.rows() / 2,
            in.cols(),
        };
        mcmat cplx_out{
            util::start_lifetime_as_array<cplx_t>(out.data(), out.size() / 2),
            out.rows() / 2,
            out.cols(),
        };
        return prox(cplx_in, cplx_out, γ);
    }

    friend real_t alpaqa_tag_invoke(tag_t<alpaqa::prox>, L1NormComplex &self,
                                    crmat in, rmat out, real_t γ) {
        return self.prox(std::move(in), std::move(out), γ);
    }
};

} // namespace alpaqa::functions
