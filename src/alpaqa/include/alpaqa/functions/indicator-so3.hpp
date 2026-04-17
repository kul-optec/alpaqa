#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/functions/prox.hpp>
#include <Eigen/LU>
#include <Eigen/SVD>
#include <cassert>

namespace alpaqa::functions {

/// Indicator function of SO(3), the group of 3D rotation matrices.
/// @ingroup grp_Functions
template <Config Conf>
struct IndicatorSO3 {
    USING_ALPAQA_CONFIG(Conf);

    /// Project a 3x3 matrix onto SO(3). Accepts both 3x3 matrices and
    /// 9-vectors as input and output (using column-major storage order).
    /// @p in and @p out may alias.
    real_t prox(crmat in, rmat out, [[maybe_unused]] real_t gamma = 1) {
        assert(in.rows() == out.rows());
        assert(in.cols() == out.cols());
        [[maybe_unused]] const bool is_3x3_matrix =
            in.rows() == 3 && in.cols() == 3;
        [[maybe_unused]] const bool is_9_vector =
            in.size() == 9 && (in.rows() == 9 || in.cols() == 9);
        assert(is_3x3_matrix || is_9_vector);

        using so3_mat = Eigen::Matrix<real_t, 3, 3>;
        Eigen::JacobiSVD<so3_mat> svd{
            in.reshaped(3, 3), Eigen::ComputeFullU | Eigen::ComputeFullV};
        so3_mat U = svd.matrixU();
        auto &&V  = svd.matrixV();
        // Ensure a proper rotation (det = +1) by flipping the smallest singular axis.
        if (U.determinant() * V.determinant() < 0)
            U.col(2) *= real_t{-1};
        out.reshaped(3, 3).noalias() = U * V.transpose();
        return 0;
    }

    friend real_t guanaqo_tag_invoke(tag_t<alpaqa::prox>, IndicatorSO3 &self,
                                     crmat in, rmat out, real_t gamma) {
        return self.prox(std::move(in), std::move(out), gamma);
    }
};

} // namespace alpaqa::functions
