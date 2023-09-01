#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/proximal/prox.hpp>
#include <cassert>
#include <cmath>
#include <stdexcept>

namespace alpaqa::proximal {

template <Config Conf, class Weight = Conf::real_t>
struct L1Norm;

template <Config Conf>
struct L1Norm<Conf, typename Conf::real_t> {
    USING_ALPAQA_CONFIG(Conf);
    using weight_t = real_t;

    L1Norm(weight_t λ = 1) : λ{λ} {
        if (λ < 0 || !std::isfinite(λ))
            throw std::invalid_argument("L1Norm::λ must be nonnegative");
    }

    weight_t λ;

    real_t prox(crmat in, rmat out, real_t γ = 1) {
        assert(λ >= 0);
        assert(in.cols() == 1);
        assert(out.cols() == 1);
        assert(in.size() == out.size());
        if (λ == 0) {
            out = in;
            return 0;
        }
        const length_t n = in.size();
        auto step        = vec::Constant(n, λ * γ);
        out              = vec::Zero(n).cwiseMax(in - step).cwiseMin(in + step);
        return λ * γ * out.template lpNorm<1>();
    }

    friend real_t alpaqa_tag_invoke(tag_t<alpaqa::prox>, L1Norm &self, crmat in,
                                    rmat out, real_t γ) {
        return self.prox(std::move(in), std::move(out), γ);
    }
};

template <Config Conf>
struct L1Norm<Conf, typename Conf::vec> {
    USING_ALPAQA_CONFIG(Conf);
    using weight_t = vec;

    L1Norm() = default;
    L1Norm(weight_t λ) : λ{λ} {
        if ((λ < 0).any() || !λ.allFinite())
            throw std::invalid_argument("L1Norm::λ must be nonnegative");
    }

    weight_t λ;

    real_t prox(crmat in, rmat out, real_t γ = 1) {
        if (λ.size() == 0) {
            out = in;
            return 0;
        }
        assert(λ >= 0);
        assert(in.cols() == 1);
        assert(out.cols() == 1);
        assert(in.size() == out.size());
        assert(in.size() == λ.size());
        const length_t n = in.size();
        auto step        = λ * γ;
        out              = vec::Zero(n).cwiseMax(in - step).cwiseMin(in + step);
        return γ * out.cwiseProduct(λ).template lpNorm<1>();
    }

    friend real_t alpaqa_tag_invoke(tag_t<alpaqa::prox>, L1Norm &self, crmat in,
                                    rmat out, real_t γ) {
        return self.prox(std::move(in), std::move(out), γ);
    }
};

} // namespace alpaqa::proximal
