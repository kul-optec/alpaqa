#include <alpaqa/config/config.hpp>
#include <gtest/gtest.h>
#include <test-util/eigen-matchers.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <ranges>
namespace vw = std::views;

#include <alpaqa/problem/box.hpp>
#include <Eigen/Sparse>

USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);
using spmat_t   = Eigen::SparseMatrix<real_t, Eigen::ColMajor, index_t>;
using mindexvec = Eigen::Map<indexvec>;
using Box       = alpaqa::Box<config_t>;

/**
 * @test
 * Algorithm for adding rows to the top of the constraint matrix (which is
 * stored as CCS). The rows we add only have a single nonzero, and correspond
 * to bound constraints.
 */
TEST(qpalm, convertConstraints) {
    length_t n = 7, m = 9;
    Box C(n);
    std::vector<index_t> bound_indices{1, 3, 4, 6};
    for (index_t i : bound_indices)
        C.upperbound(i) = 1;

    spmat_t A(m, n);
    using triplet_t = Eigen::Triplet<real_t, index_t>;
    std::vector<triplet_t> At{
        {0, 0, 2.}, {1, 0, 2.}, {8, 0, 2.}, {0, 1, 2.}, {1, 1, 2.}, {3, 1, 2.},
        {3, 2, 2.}, {3, 3, 2.}, {5, 3, 2.}, {1, 4, 2.}, {2, 4, 2.}, {4, 4, 2.},
        {6, 4, 2.}, {5, 5, 2.}, {5, 6, 2.}, {8, 6, 2.}, {2, 6, 2.},
    };
    A.setFromTriplets(At.begin(), At.end());
    spmat_t A0 = A;
    std::cout << A0 << '\n';

    // Check if the variable with the given index has bound constraints, i.e.
    // if not lowerbound == -inf and upperbound == +inf.
    auto is_bound = [&C](index_t i) {
        using std::isnan; // Assuming no NaN inputs
        return isnan(C.lowerbound(i) + C.upperbound(i)) == 0;
    };

    auto old_nnz = A.nonZeros();
    // Start by updating the outer pointer: for each active bound constraint,
    // one nonzero has to be inserted at the beginning of the current column.
    // To make space for this nonzero, all row indices and values of the current
    // column and all following columns have to be shifted. In this loop, we
    // already update the outer pointers to point to these shifted locations,
    // without actually shifting the row indices and values yet.
    // (This breaks the SparseMatrix invariants!)
    mindexvec outer_ptrs{A.outerIndexPtr(), n + 1};
    // Essentially perform outer_ptrs[1:n+1] += partial_sum(is_bound(0:n))
    index_t shift = 0;
    for (index_t col = 0; col < n; ++col) {
        shift += is_bound(col) ? 1 : 0;
        outer_ptrs(col + 1) += shift;
    }
    // We now know how many variables were constrained, so we know the new
    // number of nonzeros in the matrix, and we know how many rows to add.
    auto num_bound_constr = shift;
    // Shift down the entire matrix by changing the old row indices.
    // (This breaks the SparseMatrix invariants!)
    mindexvec{A.innerIndexPtr(), old_nnz}.array() += num_bound_constr;
    // Resize the row index and value arrays to insert the new nonzeros.
    A.resizeNonZeros(old_nnz + num_bound_constr);
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
        index_t insert_nz = is_bound(col) ? 1 : 0;
        // First recover the original outer pointer by undoing the shift.
        index_t prev_shift = shift - insert_nz;
        index_t next_outer = outer_ptrs(col + 1) - shift;
        index_t curr_outer = outer_ptrs(col) - prev_shift;
        // Then we can use the outer pointer to get the row indices and values.
        index_t *inners_ptr = A.innerIndexPtr() + curr_outer,
                *inners_end = A.innerIndexPtr() + next_outer;
        real_t *values_ptr  = A.valuePtr() + curr_outer,
               *values_end  = A.valuePtr() + next_outer;
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
    // Finally, update the number of rows of the matrix.
    A.conservativeResize(m + num_bound_constr, n);

    std::cout << A << '\n';
    EXPECT_EQ(num_bound_constr, static_cast<length_t>(bound_indices.size()));
    EXPECT_THAT(A.toDense().bottomRows(m), EigenEqual(A0.toDense()));
    mat I = mat::Zero(num_bound_constr, n);
    for (auto r = 0; index_t i : bound_indices)
        I(r++, i) = 1;
    EXPECT_THAT(A.toDense().topRows(num_bound_constr), EigenEqual(I));
}