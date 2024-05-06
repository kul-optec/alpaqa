#include "custom-config.tpp"

#include <alpaqa/implementation/inner/panoc.tpp>
#include <alpaqa/implementation/outer/alm.tpp>
#include <alpaqa/implementation/problem/type-erased-problem.tpp>
#include <alpaqa/inner/directions/panoc/noop.hpp>
#include <alpaqa/problem/box-constr-problem.hpp>

struct CustomConfig {
    /// Real scalar element type.
    using real_t = double;
    /// Complex scalar element type.
    using cplx_t = std::complex<real_t>;
    /// Dynamic vector type.
    using vec = custom::CustomVector<real_t>;
    /// Reference to mutable vector.
    using rvec = custom::CustomVectorView<real_t>;
    /// Reference to immutable vector.
    using crvec = custom::CustomVectorView<const real_t>;

    /// Type for lengths and sizes.
    using length_t = typename vec::index_type;
    /// Type for vector and matrix indices.
    using index_t = typename vec::index_type;

    /// Dummy type to indicate that index vectors and matrices are unsupported.
    struct unsupported {};

    /// Map of vector type.
    using mvec = unsupported;
    /// Immutable map of vector type.
    using cmvec = unsupported;

    /// Dynamic vector of indices.
    using indexvec = unsupported;
    /// Reference to mutable index vector.
    using rindexvec = unsupported;
    /// Reference to immutable index vector.
    using crindexvec = unsupported;
    /// Map of index vector type.
    using mindexvec = unsupported;
    /// Immutable map of index vector type.
    using cmindexvec = unsupported;

    /// Dynamic matrix type.
    using mat = unsupported;
    /// Map of matrix type.
    using mmat = unsupported;
    /// Immutable map of matrix type.
    using cmmat = unsupported;
    /// Reference to mutable matrix.
    using rmat = unsupported;
    /// Reference to immutable matrix.
    using crmat = unsupported;
    /// Dynamic complex matrix type.
    using cmat = unsupported;
    /// Map of complex matrix type.
    using mcmat = unsupported;
    /// Immutable map of complex matrix type.
    using cmcmat = unsupported;
    /// Reference to mutable complex matrix.
    using rcmat = unsupported;
    /// Reference to immutable complex matrix.
    using crcmat = unsupported;
    /// Whether indexing by vectors of indices is supported.
    static constexpr bool supports_indexvec = false;
};

template <>
struct alpaqa::is_config<CustomConfig> : std::true_type {};

template <>
inline const CustomConfig::vec alpaqa::null_vec<CustomConfig>{};

USING_ALPAQA_CONFIG(CustomConfig);

// Problem specification
// minimize  ½ xᵀQx
//  s.t.     Ax ≤ b
struct Problem : alpaqa::BoxConstrProblem<config_t> {
    alpaqa::mat<alpaqa::EigenConfigd> Q{n, n};
    alpaqa::mat<alpaqa::EigenConfigd> A{m, n};
    alpaqa::vec<alpaqa::EigenConfigd> b{m};
    mutable alpaqa::vec<alpaqa::EigenConfigd> Qx{n}, Ax{m};

    Problem() : alpaqa::BoxConstrProblem<config_t>{2, 1} {
        // Initialize problem matrices
        Q << 3, -1, -1, 3;
        A << 2, 1;
        b << -1;

        // Specify the bounds
        C.lowerbound = vec::Constant(n, -alpaqa::inf<config_t>);
        C.upperbound = vec::Constant(n, +alpaqa::inf<config_t>);
        D.lowerbound = vec::Constant(m, -alpaqa::inf<config_t>);
        D.upperbound = crvec{b.data(), b.size()};
    }

    // Evaluate the cost
    real_t eval_objective(crvec x) const {
        Qx = Q * alpaqa::cmvec<alpaqa::EigenConfigd>{x.data(), x.size()};
        return 0.5 * x.dot(crvec{Qx.data(), Qx.size()});
    }
    // Evaluat the gradient of the cost
    void eval_objective_gradient(crvec x, rvec gr) const {
        Qx = Q * alpaqa::cmvec<alpaqa::EigenConfigd>{x.data(), x.size()};
        gr = crvec{Qx.data(), Qx.size()};
    }
    // Evaluate the constraints
    void eval_constraints(crvec x, rvec g) const {
        Ax = A * alpaqa::cmvec<alpaqa::EigenConfigd>{x.data(), x.size()};
        g  = crvec{Ax.data(), Ax.size()};
    }
    // Evaluate a matrix-vector product with the gradient of the constraints
    void eval_constraints_gradient_product(crvec x, crvec y, rvec gr) const {
        (void)x;
        Qx = A.transpose() *
             alpaqa::cmvec<alpaqa::EigenConfigd>{y.data(), y.size()};
        gr = crvec{Qx.data(), Qx.size()};
    }
};

int main() {
    using Direction   = alpaqa::NoopDirection<config_t>;
    using InnerSolver = alpaqa::PANOCSolver<Direction>;
    using OuterSolver = alpaqa::ALMSolver<InnerSolver>;

    OuterSolver solver{
        {.print_interval = 1},
        {{.max_iter = 500, .print_interval = 50}, {}},
    };
    vec x = vec::Zero(2), y = vec::Zero(1);

    Problem problem;
    auto stats = solver(problem, x, y);

    return stats.status == alpaqa::SolverStatus::Converged ? 0 : 1;
}
