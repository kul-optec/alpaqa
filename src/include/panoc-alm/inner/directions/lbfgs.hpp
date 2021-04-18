#pragma once

#include <panoc-alm/inner/directions/decl/lbfgs.hpp>

namespace pa {

inline bool LBFGS::update_valid(LBFGSParams params, real_t yᵀs, real_t sᵀs,
                                real_t pᵀp) {
    // Smallest number we want to divide by without overflow
    const real_t min_divisor = std::sqrt(std::numeric_limits<real_t>::min());

    // Check if this L-BFGS update is accepted
    if (not std::isfinite(yᵀs))
        return false;
    if (sᵀs < min_divisor)
        return false;
    if (yᵀs < min_divisor)
        return false;

    // CBFGS condition: https://epubs.siam.org/doi/10.1137/S1052623499354242
    real_t α = params.cbfgs.α;
    real_t ϵ = params.cbfgs.ϵ;
    // Condition: yᵀs / sᵀs >= ϵ ‖p‖^α
    bool cbfgs_cond = yᵀs / sᵀs >= ϵ * std::pow(pᵀp, α / 2);
    if (not cbfgs_cond)
        return false;

    return true;
}

inline bool LBFGS::update(const vec &xₖ, const vec &xₖ₊₁, const vec &pₖ,
                          const vec &pₖ₊₁, Sign sign) {
    const auto s = xₖ₊₁ - xₖ;
    const auto y = sign == Sign::Positive ? pₖ₊₁ - pₖ : pₖ - pₖ₊₁;
    real_t yᵀs   = y.dot(s);
    real_t sᵀs   = s.squaredNorm();
    real_t pᵀp   = pₖ₊₁.squaredNorm();
    real_t ρ     = 1 / yᵀs;

    if (not update_valid(params, yᵀs, sᵀs, pᵀp))
        return false;

    // Store the new s and y vectors
    this->s(idx) = s;
    this->y(idx) = y;
    this->ρ(idx) = ρ;

    // Increment the index in the circular buffer
    idx = succ(idx);
    full |= idx == 0;

    return true;
}

template <class Vec>
bool LBFGS::apply(Vec &&q, real_t γ) {
    // Only apply if we have previous vectors s and y
    if (idx == 0 && not full)
        return false;

    // If the step size is negative, compute it as sᵀy/yᵀy
    if (γ < 0) {
        auto new_idx = idx > 0 ? idx - 1 : history() - 1;
        real_t yᵀy   = y(new_idx).squaredNorm();
        γ            = 1. / (ρ(new_idx) * yᵀy);
    }

    auto update1 = [&](size_t i) {
        α(i) = ρ(i) * (s(i).dot(q));
        q -= α(i) * y(i);
    };
    if (idx)
        for (size_t i = idx; i-- > 0;)
            update1(i);
    if (full)
        for (size_t i = history(); i-- > idx;)
            update1(i);

    // r ← H₀ q
    q *= γ;

    auto update2 = [&](size_t i) {
        real_t β = ρ(i) * (y(i).dot(q));
        q += (α(i) - β) * s(i);
    };
    if (full)
        for (size_t i = idx; i < history(); ++i)
            update2(i);
    for (size_t i = 0; i < idx; ++i)
        update2(i);

    return true;
}

template <class Vec, class IndexVec>
bool LBFGS::apply(Vec &&q, real_t γ, const IndexVec &J) {
    // Only apply if we have previous vectors s and y
    if (idx == 0 && not full)
        return false;

    // Eigen 3.3.9 doesn't yet support indexing using a vector of indices
    // so we'll have to do it manually
    // TODO: Abstract this away in an expression template / nullary expression?
    //       Or wait for Eigen update?

    // Dot product of two vectors, adding only the indices in set J
    auto dotJ = [&J](const auto &a, const auto &b) {
        real_t acc = 0;
        for (auto j : J)
            acc += a(j) * b(j);
        return acc;
    };

    bool at_least_one_valid_update = false;

    auto update1 = [&](size_t i) {
        // Recompute ρ, it depends on the index set J. Note that even if ρ was
        // positive for the full vectors s and y, that's not necessarily the
        // case for the smaller vectors s(J) and y(J).
        ρ(i) = 1. / dotJ(s(i), y(i));
        if (ρ(i) <= 0) // Reject negative ρ to ensure positive definiteness
            return;

        α(i) = ρ(i) * dotJ(s(i), q);
        for (auto j : J)
            q(j) -= α(i) * y(i)(j);
        at_least_one_valid_update = true; // Update with ρ > 0
    };
    if (idx)
        for (size_t i = idx; i-- > 0;)
            update1(i);
    if (full)
        for (size_t i = history(); i-- > idx;)
            update1(i);

    // If all ρ <= 0, fail
    if (not at_least_one_valid_update)
        return false;

    // Compute step size based on most recent yᵀs/yᵀy > 0
    auto newest_valid_idx = idx > 0 ? idx - 1 : history() - 1;
    while (γ < 0) {
        if (ρ(newest_valid_idx) > 0) {
            real_t yᵀy = dotJ(y(newest_valid_idx), y(newest_valid_idx));
            γ          = 1. / (ρ(newest_valid_idx) * yᵀy);
        } else {
            newest_valid_idx =
                newest_valid_idx > 0 ? newest_valid_idx - 1 : history() - 1;
        }
    }

    // r ← H₀ q
    for (auto j : J)
        q(j) *= γ;

    auto update2 = [&](size_t i) {
        if (ρ(i) <= 0)
            return;
        real_t β = ρ(i) * dotJ(y(i), q);
        for (auto j : J)
            q(j) += (α(i) - β) * s(i)(j);
    };
    if (full)
        for (size_t i = idx; i < history(); ++i)
            update2(i);
    for (size_t i = 0; i < idx; ++i)
        update2(i);

    return true;
}

inline void LBFGS::reset() {
    idx  = 0;
    full = false;
}

inline void LBFGS::resize(size_t n, size_t history) {
    sto.resize(n + 1, history * 2);
    reset();
}

inline void LBFGS::scale_y(real_t factor) {
    if (full) {
        for (size_t i = 0; i < history(); ++i) {
            y(i) *= factor;
            ρ(i) *= 1. / factor;
        }
    } else {
        for (size_t i = 0; i < idx; ++i) {
            y(i) *= factor;
            ρ(i) *= 1. / factor;
        }
    }
}

} // namespace pa

#include <panoc-alm/inner/directions/decl/panoc-direction-update.hpp>

namespace pa {

template <>
struct PANOCDirection<LBFGS> {

    static void initialize(LBFGS &lbfgs, const vec &x₀, const vec &x̂₀,
                           const vec &p₀, const vec &grad₀) {
        (void)lbfgs;
        (void)x₀;
        (void)x̂₀;
        (void)p₀;
        (void)grad₀;
    }

    static bool update(LBFGS &lbfgs, const vec &xₖ, const vec &xₖ₊₁,
                       const vec &pₖ, const vec &pₖ₊₁, const vec &grad_new,
                       const Box &C, real_t γ_new) {
        (void)grad_new;
        (void)C;
        (void)γ_new;
        return lbfgs.update(xₖ, xₖ₊₁, pₖ, pₖ₊₁, LBFGS::Sign::Negative);
    }

    static bool apply(LBFGS &lbfgs, const vec &xₖ, const vec &x̂ₖ, const vec &pₖ,
                      real_t γ, vec &qₖ) {
        (void)xₖ;
        (void)x̂ₖ;
        qₖ = pₖ;
        return lbfgs.apply(qₖ, γ);
    }

    static void changed_γ(LBFGS &lbfgs, real_t γₖ, real_t old_γₖ) {
        if (lbfgs.get_params().rescale_when_γ_changes)
            lbfgs.scale_y(γₖ / old_γₖ);
        else
            lbfgs.reset();
    }
};

} // namespace pa