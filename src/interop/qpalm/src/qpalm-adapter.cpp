#include <alpaqa/problem/sparsity-conversions.hpp>
#include <alpaqa/problem/sparsity.hpp>
#include <alpaqa/qpalm/qpalm-adapter.hpp>

#include <qpalm/sparse.hpp>

#include <cmath>
#include <stdexcept>

namespace alpaqa {

namespace {

USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);

// Check if the variable with the given index has bound constraints, i.e.
// if not lowerbound == -inf and upperbound == +inf.
bool is_bound(const Box<config_t> &C, index_t i) {
    using std::isnan; // Assuming no NaN inputs
    return isnan(C.lowerbound(i) + C.upperbound(i)) == 0;
};

/// Update the constraint matrix A, such that for each constraint C(i) with
/// finite bounds, a row is inserted into A with a one in the i-th column.
/// The newly added rows are added above the original rows of A.
/// For example, if all constraints have finite bounds, the resulting matrix
/// is @f$ \begin{pmatrix} I \\\hline A \end{pmatrix} @f$.
///
/// @pre    Assumes that the user preallocated enough space for inserting these
///         nonzero elements into A, and that A is compressed.
void add_bound_constr_to_constr_matrix(ladel_sparse_matrix &A,
                                       const Box<config_t> &C) {
    using mindexvec = Eigen::Map<Eigen::VectorX<qpalm::sp_index_t>>;

    auto m = static_cast<index_t>(A.nrow), n = static_cast<index_t>(A.ncol);
    auto old_nnz = static_cast<index_t>(A.p[n]);

    // Start by updating the outer pointer: for each active bound constraint,
    // one nonzero has to be inserted at the beginning of the current column.
    // To make space for this nonzero, all row indices and values of the current
    // column and all following columns have to be shifted. In this loop, we
    // already update the outer pointers to point to these shifted locations,
    // without actually shifting the row indices and values yet.
    // (This breaks the SparseMatrix invariants!)
    mindexvec outer_ptrs{A.p, n + 1};
    // Essentially perform outer_ptrs[1:n+1] += partial_sum(is_bound(C, 0:n))
    index_t shift = 0;
    for (index_t col = 0; col < n; ++col) {
        shift += is_bound(C, col) ? 1 : 0;
        outer_ptrs(col + 1) += shift;
    }
    // We now know how many variables were constrained, so we know the new
    // number of nonzeros in the matrix, and we know how many rows to add.
    auto num_bound_constr = shift;
    // Shift down the entire matrix by changing the old row indices.
    // (This breaks the SparseMatrix invariants!)
    mindexvec{A.i, old_nnz}.array() += num_bound_constr;
    // Now we need to make space in the row indices and value arrays, so we can
    // actually insert the nonzero elements of the rows we are adding.
    // Start with the last column, so we don't overwrite any data when shifting.
    // Throughout the iteration, the `shift` variable keeps track of how many
    // nonzeros need to be added to the current column and all previous columns.
    // The `prev_shift` variable keeps track of how many nonzeros need to be
    // added to all previous columns (excluding the current column). Note that
    // we already destroyed the original outer pointers, which we need now to
    // iterate over the original matrix. Luckily, we can recover them using
    // simple arithmetic, reversing the forward loop above.
    for (index_t col = n; col-- > 0;) {
        // Check if we need to add a nonzero in this column.
        index_t insert_nz = is_bound(C, col) ? 1 : 0;
        // First recover the original outer pointer by undoing the shift.
        index_t prev_shift = shift - insert_nz;
        index_t next_outer = outer_ptrs(col + 1) - shift;
        index_t curr_outer = outer_ptrs(col) - prev_shift;
        // Then we can use the outer pointer to get the row indices and values.
        index_t *inners_ptr = A.i + curr_outer, *inners_end = A.i + next_outer;
        real_t *values_ptr = A.x + curr_outer, *values_end = A.x + next_outer;
        // Shift over all row indices and values to make space to insert new
        // `shift` rows at the beginning of this column.
        std::shift_right(inners_ptr, inners_end + shift, shift);
        std::shift_right(values_ptr, values_end + shift, shift);
        // Set the row index and value of the row we just inserted.
        if (insert_nz) {
            inners_ptr[shift - 1] = shift - 1;
            values_ptr[shift - 1] = 1;
        }
        // Keep track of how much we should shift the previous column.
        shift = prev_shift;
    }
    // Finally, update the number of rows and nonzeros of the matrix.
    A.nrow  = m + num_bound_constr;
    A.nzmax = old_nnz + num_bound_constr;
}

/// For each constraint C(i) with finite bounds, insert these bounds into b(i),
/// followed by all bounds D, shifted by g.
void combine_bound_constr(Box<config_t> &b, const Box<config_t> &C,
                          const Box<config_t> &D, crvec g) {
    const auto n = C.lowerbound.size(), m = D.lowerbound.size();
    index_t c = 0;
    for (index_t i = 0; i < n; ++i) {
        if (is_bound(C, i)) {
            b.lowerbound(c) = C.lowerbound(i);
            b.upperbound(c) = C.upperbound(i);
            ++c;
        }
    }
    assert(c == static_cast<length_t>(b.lowerbound.size() - m));
    b.lowerbound.segment(c, m) = D.lowerbound - g;
    b.upperbound.segment(c, m) = D.upperbound - g;
}

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
    const auto n = problem.get_n(), m = problem.get_m();

    // Dummy data to evaluate Hessian and Jacobian
    vec x = vec::Zero(n), y = vec::Zero(m), g(m);

    // Construct QPALM problem
    OwningQPALMData qp;

    using SparseCSC    = sparsity::SparseCSC<config_t, qpalm::sp_index_t>;
    using Sparsity     = sparsity::Sparsity<config_t>;
    using SparsityConv = sparsity::SparsityConverter<Sparsity, SparseCSC>;
    { // Evaluate cost Hessian
        Sparsity sp_Q_orig = problem.get_hess_L_sparsity();
        SparsityConv sp_Q{sp_Q_orig, {.order = SparseCSC::SortedRows}};
        auto nnz_Q = static_cast<qpalm::sp_index_t>(sp_Q.get_sparsity().nnz());
        auto symm  = convert_symmetry(sp_Q.get_sparsity().symmetry);
        qp.sto->Q  = qpalm::ladel_sparse_create(n, n, nnz_Q, symm);
        qp.Q       = qp.sto->Q.get();
        // Copy sparsity pattern
        std::ranges::copy(sp_Q.get_sparsity().inner_idx, qp.Q->i);
        std::ranges::copy(sp_Q.get_sparsity().outer_ptr, qp.Q->p);
        // Get actual values
        mvec H_values{qp.Q->x, nnz_Q};
        auto eval_h = [&](rvec v) { problem.eval_hess_L(x, y, 1, v); };
        sp_Q.convert_values(eval_h, H_values);
    }
    { // Evaluate constraints Jacobian
        Sparsity sp_A_orig = problem.get_jac_g_sparsity();
        SparsityConv sp_A{sp_A_orig, {.order = SparseCSC::SortedRows}};
        auto nnz_A = static_cast<qpalm::sp_index_t>(sp_A.get_sparsity().nnz());
        auto symm  = convert_symmetry(sp_A.get_sparsity().symmetry);
        qp.sto->A  = qpalm::ladel_sparse_create(m, n, nnz_A + n, symm);
        qp.A       = qp.sto->A.get();
        // Copy sparsity pattern
        std::ranges::copy(sp_A.get_sparsity().inner_idx, qp.A->i);
        std::ranges::copy(sp_A.get_sparsity().outer_ptr, qp.A->p);
        // Get actual values
        mvec J_values{qp.A->x, nnz_A};
        auto eval_j = [&](rvec v) { problem.eval_jac_g(x, v); };
        sp_A.convert_values(eval_j, J_values);
        // Add the bound constraints
        add_bound_constr_to_constr_matrix(*qp.A, problem.get_box_C());
    }
    { // Evaluate constraints
        problem.eval_g(x, g);
    }
    { // Evaluate cost and cost gradient
        qp.sto->q.resize(n);
        qp.q = qp.sto->q.data();
        qp.c = problem.eval_f_grad_f(x, qp.sto->q);
    }
    { // Combine bound constraints
        qp.sto->b.lowerbound.resize(qp.A->nrow);
        qp.sto->b.upperbound.resize(qp.A->nrow);
        qp.bmin = qp.sto->b.lowerbound.data();
        qp.bmax = qp.sto->b.upperbound.data();
        // Combine bound constraints and linear constraints
        auto &&C = problem.get_box_C(), &&D = problem.get_box_D();
        combine_bound_constr(qp.sto->b, C, D, g);
    }
    qp.m = static_cast<size_t>(qp.A->nrow);
    qp.n = static_cast<size_t>(qp.Q->nrow);
    return qp;
}

} // namespace alpaqa