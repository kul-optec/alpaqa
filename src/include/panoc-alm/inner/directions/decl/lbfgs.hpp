#pragma once

#include <panoc-alm/util/box.hpp>
#include <panoc-alm/util/vec.hpp>

#include <panoc-alm/inner/directions/decl/lbfgs-fwd.hpp>

namespace pa {

/// Parameters for the @ref LBFGS and @ref SpecializedLBFGS classes.
struct LBFGSParams {
    struct {
        real_t α = 1;
        real_t ϵ = 1e-10;
    }
    /// Parameters in the cautious BFGS update condition
    /// @f[ \frac{y^\top s}{s^\top s} \ge \epsilon \| g \|^\alpha @f]
    /// @see https://epubs.siam.org/doi/10.1137/S1052623499354242
    cbfgs;

    bool rescale_when_γ_changes = false;
};

/// Limited memory Broyden–Fletcher–Goldfarb–Shanno (L-BFGS) algorithm
/// @ingroup  grp_PANOCDirectionProviders
class LBFGS {
  public:
    using Params = LBFGSParams;

    /// The sign of the vectors @f$ p @f$ passed to the @ref LBFGS::update
    /// method.
    enum class Sign {
        Positive, ///< @f$ p \sim \nabla \psi(x) @f$
        Negative, ///< @f$ p \sim -\nabla \psi(x) @f$
    };

    LBFGS(Params params) : params(params) {}
    LBFGS(Params params, size_t n, size_t history) : params(params) {
        resize(n, history);
    }

    /// Check if the new vectors s and y allow for a valid BFGS update that
    /// preserves the positive definiteness of the Hessian approximation.
    static bool update_valid(LBFGSParams params, real_t yᵀs, real_t sᵀs,
                             real_t pᵀp);

    /// Update the inverse Hessian approximation using the new vectors xₖ₊₁
    /// and pₖ₊₁.
    bool update(const vec &xₖ, const vec &xₖ₊₁, const vec &pₖ, const vec &pₖ₊₁,
                Sign sign);

    /// Apply the inverse Hessian approximation to the given vector q.
    template <class Vec>
    void apply(Vec &&q, real_t γ);

    /// Apply the inverse Hessian approximation to the given vector q, applying
    /// only the columns and rows of the Hessian in the index set J.
    template <class Vec, class IndexVec>
    void apply(Vec &&q, real_t γ, const IndexVec &indices);

    /// Throw away the approximation and all previous vectors s and y.
    void reset();
    /// Re-allocate storage for a problem with a different size. Causes
    /// a @ref reset.
    void resize(size_t n, size_t history);

    /// Scale the stored y vectors by the given factor.
    void scale_y(real_t factor);

    std::string get_name() const { return "LBFGS"; }

    const Params &get_params() const { return params; }

    /// Get the size of the s and y vectors in the buffer.
    size_t n() const { return sto.rows() - 1; }
    /// Get the number of previous vectors s and y stored in the buffer.
    size_t history() const { return sto.cols() / 2; }
    /// Get the next index in the circular buffer of previous s and y vectors.
    size_t succ(size_t i) const { return i + 1 < history() ? i + 1 : 0; }

    auto s(size_t i) { return sto.col(2 * i).topRows(n()); }
    auto s(size_t i) const { return sto.col(2 * i).topRows(n()); }
    auto y(size_t i) { return sto.col(2 * i + 1).topRows(n()); }
    auto y(size_t i) const { return sto.col(2 * i + 1).topRows(n()); }
    real_t &ρ(size_t i) { return sto.coeffRef(n(), 2 * i); }
    const real_t &ρ(size_t i) const { return sto.coeff(n(), 2 * i); }
    real_t &α(size_t i) { return sto.coeffRef(n(), 2 * i + 1); }
    const real_t &α(size_t i) const { return sto.coeff(n(), 2 * i + 1); }

  private:
    using storage_t = Eigen::Matrix<real_t, Eigen::Dynamic, Eigen::Dynamic>;

    storage_t sto;
    size_t idx = 0;
    bool full  = false;
    Params params;
};

} // namespace pa