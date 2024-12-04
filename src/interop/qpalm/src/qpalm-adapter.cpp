#include <alpaqa/problem/sparsity-conversions.hpp>
#include <alpaqa/problem/sparsity.hpp>
#include <alpaqa/qpalm/qpalm-adapter.hpp>
#include <alpaqa/util/lin-constr-converter.hpp>

#include <qpalm/sparse.hpp>

#include <cmath>
#include <stdexcept>

namespace alpaqa {

namespace {

USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);

int convert_symmetry(sparsity::Symmetry symmetry) {
    switch (symmetry) {
        case sparsity::Symmetry::Unsymmetric: return UNSYMMETRIC;
        case sparsity::Symmetry::Upper: return UPPER;
        case sparsity::Symmetry::Lower: return LOWER;
        default: throw std::invalid_argument("Invalid symmetry");
    }
}

} // namespace

OwningQPALMData
build_qpalm_problem(const TypeErasedProblem<EigenConfigd> &problem) {
    USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);

    // Get the dimensions of the problem matrices
    const auto n = problem.get_num_variables(),
               m = problem.get_num_constraints();

    // Dummy data to evaluate Hessian and Jacobian
    vec x = vec::Zero(n), y = vec::Zero(m), g(m);

    // Construct QPALM problem
    OwningQPALMData qp;

    using std::span;
    using qp_idx_t     = qpalm::sp_index_t;
    using SparseCSC    = sparsity::SparseCSC<qp_idx_t, qp_idx_t>;
    using SparsityConv = sparsity::SparsityConverter<Sparsity, SparseCSC>;
    using ConstrConv   = LinConstrConverter<config_t, qp_idx_t, qp_idx_t>;
    { // Evaluate cost Hessian
        Sparsity sp_Q_orig = problem.get_lagrangian_hessian_sparsity();
        SparsityConv sp_Q{sp_Q_orig, {.order = SparseCSC::SortedRows}};
        auto nnz_Q = static_cast<qp_idx_t>(sp_Q.get_sparsity().nnz());
        auto symm  = convert_symmetry(sp_Q.get_sparsity().symmetry);
        qp.sto->Q  = qpalm::ladel_sparse_create(n, n, nnz_Q, symm);
        qp.Q       = qp.sto->Q.get();
        // Copy sparsity pattern
        std::ranges::copy(sp_Q.get_sparsity().inner_idx, qp.Q->i);
        std::ranges::copy(sp_Q.get_sparsity().outer_ptr, qp.Q->p);
        // Get actual values
        mvec H_values{qp.Q->x, static_cast<index_t>(nnz_Q)};
        sp_Q.convert_values_into(as_span(H_values), [&](std::span<real_t> v) {
            problem.eval_lagrangian_hessian(x, y, 1, as_vec(v));
        });
    }
    { // Evaluate constraints Jacobian
        Sparsity sp_A_orig = problem.get_constraints_jacobian_sparsity();
        SparsityConv sp_A{sp_A_orig, {.order = SparseCSC::SortedRows}};
        auto nnz_A = static_cast<qp_idx_t>(sp_A.get_sparsity().nnz());
        auto symm  = convert_symmetry(sp_A.get_sparsity().symmetry);
        qp.sto->A  = qpalm::ladel_sparse_create(m, n, nnz_A + n, symm);
        qp.A       = qp.sto->A.get();
        // Copy sparsity pattern
        std::ranges::copy(sp_A.get_sparsity().inner_idx, qp.A->i);
        std::ranges::copy(sp_A.get_sparsity().outer_ptr, qp.A->p);
        // Get actual values
        mvec J_values{qp.A->x, static_cast<index_t>(nnz_A)};
        sp_A.convert_values_into(as_span(J_values), [&](std::span<real_t> v) {
            problem.eval_constraints_jacobian(x, as_vec(v));
        });

        // Add the bound constraints
        ConstrConv::SparseView A{
            .nrow      = qp.A->nrow,
            .ncol      = qp.A->ncol,
            .inner_idx = span{qp.A->i, static_cast<size_t>(qp.A->nzmax)},
            .outer_ptr = span{qp.A->p, static_cast<size_t>(qp.A->ncol) + 1},
            .values    = span{qp.A->x, static_cast<size_t>(qp.A->nzmax)},
        };
        ConstrConv::add_box_constr_to_constr_matrix(
            A, problem.get_variable_bounds());
        qp.A->nrow = A.nrow;
    }
    { // Evaluate constraints
        problem.eval_constraints(x, g);
    }
    { // Evaluate cost and cost gradient
        qp.sto->q.resize(n);
        qp.q = qp.sto->q.data();
        qp.c = problem.eval_objective_and_gradient(x, qp.sto->q);
    }
    { // Combine bound constraints
        qp.sto->b.lower.resize(qp.A->nrow);
        qp.sto->b.upper.resize(qp.A->nrow);
        qp.bmin = qp.sto->b.lower.data();
        qp.bmax = qp.sto->b.upper.data();
        // Combine bound constraints and linear constraints
        auto &&C = problem.get_variable_bounds(),
             &&D = problem.get_general_bounds();
        ConstrConv::combine_bound_constr(C, D, qp.sto->b, g);
    }
    qp.m = static_cast<size_t>(qp.A->nrow);
    qp.n = static_cast<size_t>(qp.Q->nrow);
    return qp;
}

} // namespace alpaqa