#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/export.hpp>
#include <alpaqa/inner/directions/panoc-direction-update.hpp>
#include <alpaqa/inner/internal/panoc-helpers.hpp>
#include <alpaqa/problem/sparsity-conversions.hpp>
#include <alpaqa/problem/sparsity.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>
#include <alpaqa/util/alloc-check.hpp>
#include <alpaqa/util/index-set.hpp>
#include <alpaqa/util/print.hpp>
#include <iostream>
#include <optional>
#include <stdexcept>

#include <Eigen/Cholesky>
#include <Eigen/Eigenvalues>
#include <Eigen/src/Cholesky/LDLT.h>

namespace alpaqa {

/// Parameters for the @ref ConvexNewtonDirection class.
/// @ingroup grp_Parameters
template <Config Conf>
struct ConvexNewtonRegularizationParams {
    USING_ALPAQA_CONFIG(Conf);
    real_t ζ  = real_t(1e-8);
    real_t ν  = 1;
    bool ldlt = false;
};

/// Parameters for the @ref ConvexNewtonDirection class.
/// @ingroup grp_Parameters
template <Config Conf>
struct ConvexNewtonDirectionParams {
    USING_ALPAQA_CONFIG(Conf);
    /// Set this option to a nonzero value to include the Hessian-vector product
    /// @f$ \nabla^2_{x_\mathcal{J}x_\mathcal{K}}\psi(x) q_\mathcal{K} @f$ from
    /// equation 12b in @cite pas2022alpaqa, scaled by this parameter.
    /// Set it to zero to leave out that term.
    real_t hessian_vec_factor = 0;
    bool quadratic            = false;
};

/// @ingroup grp_DirectionProviders
template <Config Conf = DefaultConfig>
struct ConvexNewtonDirection {
    USING_ALPAQA_CONFIG(Conf);
    using Problem           = TypeErasedProblem<config_t>;
    using DirectionParams   = ConvexNewtonDirectionParams<config_t>;
    using AcceleratorParams = ConvexNewtonRegularizationParams<config_t>;

    struct Params {
        AcceleratorParams accelerator = {};
        DirectionParams direction     = {};
    };

    ConvexNewtonDirection() = default;
    ConvexNewtonDirection(const Params &params)
        : reg_params(params.accelerator), direction_params(params.direction) {}
    ConvexNewtonDirection(const AcceleratorParams &params,
                          const DirectionParams &directionparams = {})
        : reg_params(params), direction_params(directionparams) {}

    /// @see @ref PANOCDirection::initialize
    void initialize(const Problem &problem, [[maybe_unused]] crvec y,
                    [[maybe_unused]] crvec Σ, [[maybe_unused]] real_t γ_0,
                    [[maybe_unused]] crvec x_0, [[maybe_unused]] crvec x̂_0,
                    [[maybe_unused]] crvec p_0,
                    [[maybe_unused]] crvec grad_ψx_0) {
        if (problem.get_m() != 0)
            throw std::invalid_argument("Convex Newton direction does not "
                                        "support general constraints");
        if (!problem.provides_eval_inactive_indices_res_lna())
            throw std::invalid_argument("Convex Newton direction requires "
                                        "eval_inactive_indices_res_lna");
        if (!problem.provides_eval_hess_L())
            throw std::invalid_argument("Structured Newton requires "
                                        "eval_hess_L");
        // Store reference to problem
        this->problem = &problem;
        // Allocate workspaces
        const auto n = problem.get_n();
        JK.resize(n);
        JK_old.resize(n);
        nJ_old = -1;
        H.resize(n, n);
        HJ_storage.resize(n * n);
        work.resize(n);
        auto sparsity = problem.get_hess_ψ_sparsity();
        if (!is_dense(sparsity))
            std::cerr << "Sparse hessians not yet implemented, converting to "
                         "dense matrix (may be very slow)\n";
        H_sparsity.emplace(sparsity);
        have_hess = false;
    }

    /// @see @ref PANOCDirection::has_initial_direction
    bool has_initial_direction() const { return true; }

    /// @see @ref PANOCDirection::update
    bool update([[maybe_unused]] real_t γₖ, [[maybe_unused]] real_t γₙₑₓₜ,
                [[maybe_unused]] crvec xₖ, [[maybe_unused]] crvec xₙₑₓₜ,
                [[maybe_unused]] crvec pₖ, [[maybe_unused]] crvec pₙₑₓₜ,
                [[maybe_unused]] crvec grad_ψxₖ,
                [[maybe_unused]] crvec grad_ψxₙₑₓₜ) {
        return true;
    }

    template <class Solver>
    void solve(rmat H, rvec q) const {
        ScopedMallocAllower ma;
        Solver ll{H};
        if (ll.info() != Eigen::Success)
            throw std::runtime_error("Cholesky factorization failed. "
                                     "Is the problem convex?");
        ll.solveInPlace(q);
    }

    /// @see @ref PANOCDirection::apply
    bool apply(real_t γₖ, crvec xₖ, [[maybe_unused]] crvec x̂ₖ, crvec pₖ,
               crvec grad_ψxₖ, rvec qₖ) const {
        length_t n = xₖ.size();
        // Evaluate the Hessian
        if (!have_hess) {
            const auto &y = null_vec<config_t>;
            auto eval_h = [&](rvec v) { problem->eval_hess_L(xₖ, y, 1, v); };
            H_sparsity->convert_values(eval_h, H.reshaped());
            have_hess = direction_params.quadratic;
        }
        // Find inactive indices J
        auto nJ = problem->eval_inactive_indices_res_lna(γₖ, xₖ, grad_ψxₖ, JK);
        auto J  = JK.topRows(nJ);
        auto HJ = HJ_storage.topRows(nJ * nJ).reshaped(nJ, nJ);
        HJ      = H(J, J);
        // Regularize the Hessian
        real_t res_sq = pₖ.squaredNorm() / (γₖ * γₖ);
        real_t reg    = reg_params.ζ * std::pow(res_sq, reg_params.ν / 2);
        HJ += reg * mat::Identity(nJ, nJ);
        // Compute the right-hand side
        qₖ     = pₖ;
        rvec w = work.topRows(nJ);
        if (direction_params.hessian_vec_factor != 0) {
            rindexvec K = JK.bottomRows(n - nJ);
            detail::IndexSet<config_t>::compute_complement(J, K, n);
            w = (real_t(1) / γₖ) * pₖ(J) -
                direction_params.hessian_vec_factor * (H(J, K) * qₖ(K));
        } else {
            w = (real_t(1) / γₖ) * pₖ(J);
        }
        // Solve the system
        if (H_sparsity->get_sparsity().symmetry == sparsity::Symmetry::Upper)
            reg_params.ldlt ? solve<Eigen::LDLT<rmat, Eigen::Upper>>(HJ, w)
                            : solve<Eigen::LLT<rmat, Eigen::Upper>>(HJ, w);
        else
            reg_params.ldlt ? solve<Eigen::LDLT<rmat, Eigen::Lower>>(HJ, w)
                            : solve<Eigen::LLT<rmat, Eigen::Lower>>(HJ, w);
        qₖ(J) = w;
        return true;
    }

    /// @see @ref PANOCDirection::changed_γ
    void changed_γ([[maybe_unused]] real_t γₖ, [[maybe_unused]] real_t old_γₖ) {
    }

    /// @see @ref PANOCDirection::reset
    void reset() {}

    /// @see @ref PANOCDirection::get_name
    std::string get_name() const {
        return "ConvexNewtonDirection<" + std::string(config_t::get_name()) +
               '>';
    }

    const auto &get_params() const { return direction_params; }

  private:
    const Problem *problem = nullptr;

    mutable indexvec JK, JK_old;
    mutable index_t nJ_old = -1;
    mutable mat H;
    using sp_conv_t = sparsity::SparsityConverter<Sparsity<config_t>,
                                                  sparsity::Dense<config_t>>;
    mutable std::optional<sp_conv_t> H_sparsity;
    mutable vec HJ_storage, work;
    mutable bool have_hess = false;

  public:
    AcceleratorParams reg_params;
    DirectionParams direction_params;
};

// clang-format off
ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, ConvexNewtonDirection, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, ConvexNewtonDirection, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, ConvexNewtonDirection, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(struct, ConvexNewtonDirection, EigenConfigq);)
// clang-format on

} // namespace alpaqa
