#pragma once

#include <alpaqa/problem/box.hpp>
#include <alpaqa/problem/sparsity.hpp>
#include <alpaqa/util/span.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <span>

namespace alpaqa {

template <Config Conf, class IndexT, class StorageIndexT>
struct LinConstrConverter {
    using config_t        = Conf;
    using real_t          = typename config_t::real_t;
    using index_t         = IndexT;
    using storage_index_t = StorageIndexT;

    struct SparseView {
        index_t nrow, ncol;
        std::span<index_t> inner_idx;
        std::span<storage_index_t> outer_ptr;
        std::span<real_t> values;
    };

    /// Check if the variable with the given index has bound constraints, i.e.
    /// if not lower == -inf and upper == +inf.
    static bool is_bound(std::span<const real_t> lbx,
                         std::span<const real_t> ubx, size_t i) {
        using std::isnan; // Assuming no NaN inputs
        return isnan(lbx[i] + ubx[i]) == 0;
    }
    static bool is_bound(const sets::Box<config_t> &C,
                         typename config_t::index_t i) {
        return is_bound(as_span(C.lower), as_span(C.upper),
                        static_cast<size_t>(i));
    }

    static index_t count_bounds(std::span<const real_t> lbx,
                                std::span<const real_t> ubx) {
        auto n = static_cast<index_t>(lbx.size());
        assert(static_cast<index_t>(ubx.size()) == n);
        index_t shift = 0;
        for (index_t col = 0; col < n; ++col)
            shift += is_bound(lbx, ubx, col) ? 1 : 0;
        return shift;
    }

    static index_t count_bounds(const sets::Box<config_t> &C) {
        return count_bounds(as_span(C.lower), as_span(C.upper));
    }

    static void add_box_constr_to_constr_matrix(mat<config_t> &A,
                                                std::span<const real_t> lbx,
                                                std::span<const real_t> ubx) {
        auto n = A.cols(), m = A.rows();
        index_t shift = count_bounds(lbx, ubx);
        mat<config_t> B(shift + m, n);
        B << mat<config_t>::Zero(shift, n), A;
        shift = 0;
        for (index_t col = 0; col < n; ++col)
            if (is_bound(lbx, ubx, col))
                B(shift++, col) = 1;
        using std::swap;
        swap(A, B);
    }
    static void add_box_constr_to_constr_matrix(mat<config_t> &A,
                                                const sets::Box<config_t> &C) {
        return add_box_constr_to_constr_matrix(A, as_span(C.lower),
                                               as_span(C.upper));
    }

    static void
    add_box_constr_to_constr_matrix_inplace(index_t n_row, rmat<config_t> A,
                                            std::span<const real_t> lbx,
                                            std::span<const real_t> ubx) {
        index_t n = A.cols(), shift = A.rows() - n_row;
        // Shift down the entire matrix in-place
        for (index_t col = n; col-- > 0;)
            std::ranges::reverse_copy(
                A.col(col).topRows(n_row),
                std::reverse_iterator{A.data() + (col + 1) * A.rows()});
        // Add ones in the top block
        A.topRows(shift).setZero();
        shift = 0;
        for (index_t col = 0; col < n; ++col)
            if (is_bound(lbx, ubx, col))
                A(shift++, col) = 1;
        assert(shift == A.rows() - n_row);
    }
    static void
    add_box_constr_to_constr_matrix_inplace(index_t n_row, rmat<config_t> A,
                                            const sets::Box<config_t> &C) {
        return add_box_constr_to_constr_matrix_inplace(
            n_row, A, as_span(C.lower), as_span(C.upper));
    }

    static void add_box_constr_to_constr_matrix_inplace_vec(
        index_t n_row, index_t n_col, rvec<config_t> A,
        std::span<const real_t> lbx, std::span<const real_t> ubx) {
        assert(A.size() % n_col == 0);
        index_t tot_rows = A.size() / n_col;
        index_t shift    = tot_rows - n_row;
        // Shift down the entire matrix in-place
        auto A_old = A.topRows(n_row * n_col).reshaped(n_row, n_col);
        auto A_new = A.reshaped(tot_rows, n_col);
        for (index_t col = n_col; col-- > 0;)
            std::ranges::reverse_copy(
                A_old.col(col).topRows(n_row),
                std::reverse_iterator{A.data() + (col + 1) * A_new.rows()});
        // Add ones in the top block
        A_new.topRows(shift).setZero();
        shift = 0;
        for (index_t col = 0; col < n_col; ++col)
            if (is_bound(lbx, ubx, col))
                A_new(shift++, col) = 1;
        assert(shift == A_new.rows() - n_row);
    }
    static void
    add_box_constr_to_constr_matrix_inplace_vec(index_t n_row, index_t n_col,
                                                rvec<config_t> A,
                                                const sets::Box<config_t> &C) {
        return add_box_constr_to_constr_matrix_inplace_vec(
            n_row, n_col, A, as_span(C.lower), as_span(C.upper));
    }

    /// Update the constraint matrix A, such that for each constraint C(i) with
    /// finite bounds, a row is inserted into A with a one in the i-th column.
    /// The newly added rows are added above the original rows of A.
    /// For example, if all constraints have finite bounds, the resulting matrix
    /// is @f$ \begin{pmatrix} I \\\hline A \end{pmatrix} @f$.
    ///
    /// @pre    Assumes that the user preallocated enough space for inserting
    ///         these nonzero elements into A, and that A is compressed.
    static void add_box_constr_to_constr_matrix(SparseView &A,
                                                std::span<const real_t> lbx,
                                                std::span<const real_t> ubx);
    static void add_box_constr_to_constr_matrix(SparseView &A,
                                                const sets::Box<config_t> &C) {
        return add_box_constr_to_constr_matrix(A, as_span(C.lower),
                                               as_span(C.upper));
    }

    /// For each constraint lbx(i)/ubx(i) with finite bounds, insert these
    /// bounds into new_lbg(i)/new_ubg(i), followed by all bounds lbg(i)/ubg(i),
    /// shifted by the constant vector -g₀.
    static void combine_bound_constr(std::span<const real_t> lbx,
                                     std::span<const real_t> ubx,
                                     std::span<const real_t> lbg,
                                     std::span<const real_t> ubg,
                                     std::span<real_t> new_lbg,
                                     std::span<real_t> new_ubg,
                                     std::span<const real_t> g0);
    static void combine_bound_constr(const sets::Box<config_t> &C,
                                     const sets::Box<config_t> &D,
                                     sets::Box<config_t> &new_D,
                                     typename config_t::crvec g0) {
        return combine_bound_constr(as_span(C.lower), as_span(C.upper),
                                    as_span(D.lower), as_span(D.upper),
                                    as_span(new_D.lower), as_span(new_D.upper),
                                    as_span(g0));
    }
    static void combine_bound_constr(const sets::Box<config_t> &C,
                                     const sets::Box<config_t> &D,
                                     std::span<real_t> new_lbg,
                                     std::span<real_t> new_ubg,
                                     typename config_t::crvec g0) {
        return combine_bound_constr(as_span(C.lower), as_span(C.upper),
                                    as_span(D.lower), as_span(D.upper), new_lbg,
                                    new_ubg, as_span(g0));
    }
};

template <Config Conf, class IndexT, class StorageIndexT>
void LinConstrConverter<Conf, IndexT, StorageIndexT>::
    add_box_constr_to_constr_matrix(SparseView &A, std::span<const real_t> lbx,
                                    std::span<const real_t> ubx) {
    auto n = static_cast<size_t>(A.ncol);
    assert(A.outer_ptr.size() == static_cast<size_t>(n) + 1);
    auto old_nnz = A.outer_ptr[n];

    // Start by updating the outer pointer: for each active bound constraint,
    // one nonzero has to be inserted at the beginning of the current column.
    // To make space for this nonzero, all row indices and values of the current
    // column and all following columns have to be shifted. In this loop, we
    // already update the outer pointers to point to these shifted locations,
    // without actually shifting the row indices and values yet.
    // (This breaks the SparseMatrix invariants!)
    storage_index_t shift = 0;
    // Essentially perform outer_ptrs[1:n+1] += partial_sum(is_bound(C, 0:n))
    for (size_t col = 0; col < n; ++col) {
        shift += is_bound(lbx, ubx, col) ? 1 : 0;
        A.outer_ptr[col + 1] += shift;
    }
    assert(A.inner_idx.size() >= static_cast<size_t>(old_nnz + shift));
    // We now know how many variables were constrained, so we know the new
    // number of nonzeros in the matrix, and we know how many rows to add.
    auto num_bound_constr = static_cast<index_t>(shift);
    // Shift down the entire matrix by changing the old row indices.
    // (This breaks the SparseMatrix invariants!)
    for (index_t &i : A.inner_idx.first(static_cast<size_t>(old_nnz)))
        i += num_bound_constr;
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
    for (size_t col = n; col-- > 0;) {
        // Check if we need to add a nonzero in this column.
        storage_index_t insert_nz = is_bound(lbx, ubx, col) ? 1 : 0;
        // First recover the original outer pointer by undoing the shift.
        storage_index_t prev_shift = shift - insert_nz;
        storage_index_t next_outer = A.outer_ptr[col + 1] - shift;
        storage_index_t curr_outer = A.outer_ptr[col] - prev_shift;
        // Then we can use the outer pointer to get the row indices and values.
        auto inners_ptr = A.inner_idx.begin() + curr_outer,
             inners_end = A.inner_idx.begin() + next_outer;
        auto values_ptr = A.values.begin() + curr_outer,
             values_end = A.values.begin() + next_outer;
        // Shift over all row indices and values to make space to insert new
        // `shift` rows at the beginning of this column.
        std::shift_right(inners_ptr, inners_end + shift, shift);
        std::shift_right(values_ptr, values_end + shift, shift);
        // Set the row index and value of the row we just inserted.
        if (insert_nz) {
            inners_ptr[shift - 1] = static_cast<index_t>(shift) - 1;
            values_ptr[shift - 1] = 1;
        }
        // Keep track of how much we should shift the previous column.
        shift = prev_shift;
    }
    // Finally, update the number of rows of the matrix.
    A.nrow += num_bound_constr;
}

template <Config Conf, class IndexT, class StorageIndexT>
void LinConstrConverter<Conf, IndexT, StorageIndexT>::combine_bound_constr(
    std::span<const real_t> lbx, std::span<const real_t> ubx,
    std::span<const real_t> lbg, std::span<const real_t> ubg,
    std::span<real_t> new_lbg, std::span<real_t> new_ubg,
    std::span<const real_t> g0) {
    const auto n = lbx.size(), m [[maybe_unused]] = lbg.size();
    assert(ubx.size() == n);
    assert(ubg.size() == m);
    assert(g0.size() == m);
    size_t c = 0;
    for (size_t i = 0; i < n; ++i) {
        if (is_bound(lbx, ubx, i)) {
            new_lbg[c] = lbx[i];
            new_ubg[c] = ubx[i];
            ++c;
        }
    }
    assert(c + m == new_lbg.size());
    std::ranges::transform(lbg, g0, &new_lbg[c], std::minus{});
    std::ranges::transform(ubg, g0, &new_ubg[c], std::minus{});
}

} // namespace alpaqa