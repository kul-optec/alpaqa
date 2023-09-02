#pragma once

#include <alpaqa/config/config.hpp>
#include <Eigen/SVD>

namespace alpaqa::functions {

template <Config Conf>
using DefaultSVD = Eigen::BDCSVD<typename Conf::mat,
                                 Eigen::ComputeThinU | Eigen::ComputeThinV>;

/// Nuclear norm (ℓ₁-norm of singular values).
/// @ingroup grp_Functions
template <Config Conf, class SVD = DefaultSVD<Conf>>
struct NuclearNorm {
    USING_ALPAQA_CONFIG(Conf);

    /// Construct without pre-allocation.
    NuclearNorm(real_t λ = 1) : λ{λ} {
        if (λ < 0 || !std::isfinite(λ))
            throw std::invalid_argument("NuclearNorm::λ must be nonnegative");
    }
    /// Construct with pre-allocation.
    NuclearNorm(real_t λ, length_t rows, length_t cols)
        : λ{λ}, svd{rows, cols}, singular_values{svd.singularValues().size()} {
        if (λ < 0 || !std::isfinite(λ))
            throw std::invalid_argument("NuclearNorm::λ must be nonnegative");
    }

    real_t λ;
    SVD svd;
    vec singular_values;

    real_t prox(crmat in, rmat out, real_t γ = 1) {
        if (λ == 0) {
            out = in;
            return 0;
        }
        svd.compute(in);
        const length_t n = svd.singularValues().size();
#if 0
        L1Norm<config_t> l1 = λ;
        real_t value = l1.prox(svd.singularValues(), singular_values, γ);
#else
        auto step        = vec::Constant(n, λ * γ);
        singular_values  = vec::Zero(n).cwiseMax(svd.singularValues() - step);
        real_t value     = λ * γ * singular_values.template lpNorm<1>();
#endif
        using std::ranges::find;
        index_t rank = find(singular_values, 0) - singular_values.begin();
        using Eigen::placeholders::all, Eigen::seqN;
        auto sel = seqN(0, rank);
        auto &&U = svd.matrixU(), &&V = svd.matrixV();
        out.noalias() = U(all, sel) * singular_values(sel).asDiagonal() *
                        V.transpose()(sel, all);
        return value;
    }

    friend real_t alpaqa_tag_invoke(tag_t<alpaqa::prox>, NuclearNorm &self,
                                    crmat in, rmat out, real_t γ) {
        return self.prox(std::move(in), std::move(out), γ);
    }
};

} // namespace alpaqa::functions