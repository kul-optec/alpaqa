#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/sparsity-conversions.hpp>
#include <alpaqa/problem/sparsity.hpp>
namespace sp = alpaqa::sparsity;

#include <Eigen/Sparse>
#include <gtest/gtest.h>
#include <test-util/eigen-matchers.hpp>
#include <iostream>
#include <vector>

USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);

auto cvt_values_func(crvec m) {
    return [m](rvec v) { v = m; };
}

TEST(Sparsity, convertDenseToCOO) {
    using Source      = sp::Dense<config_t>;
    using Result      = sp::SparseCOO<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    Source dense{
        .rows     = 3,
        .cols     = 4,
        .symmetry = sp::Symmetry::Unsymmetric,
    };
    converter_t converter = Sparsity{dense};
    const auto &result    = converter.get_sparsity();
    ASSERT_EQ(result.nnz(), 12);
    indexvec expected_row_idcs(12), expected_col_idcs(12);
    expected_row_idcs << 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2;
    expected_col_idcs << 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3;
    EXPECT_THAT(result.row_indices, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.col_indices, EigenEqual(expected_col_idcs));
    mat m(3, 4);
    m << 11, 12, 13, 14, 21, 22, 23, 24, 31, 32, 33, 34;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m.reshaped()), v);
    EXPECT_THAT(v, EigenEqual(m.reshaped()));
}

TEST(Sparsity, convertDenseToCOOfirstIndex) {
    using Source      = sp::Dense<config_t>;
    using Result      = sp::SparseCOO<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    Source dense{
        .rows     = 3,
        .cols     = 4,
        .symmetry = sp::Symmetry::Unsymmetric,
    };
    converter_t converter{Sparsity{dense}, {.first_index = 1}};
    const auto &result = converter.get_sparsity();
    ASSERT_EQ(result.nnz(), 12);
    indexvec expected_row_idcs(12), expected_col_idcs(12);
    expected_row_idcs << 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3;
    expected_col_idcs << 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4;
    EXPECT_THAT(result.row_indices, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.col_indices, EigenEqual(expected_col_idcs));
    mat m(3, 4);
    m << 11, 12, 13, 14, 21, 22, 23, 24, 31, 32, 33, 34;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m.reshaped()), v);
    EXPECT_THAT(v, EigenEqual(m.reshaped()));
}

TEST(Sparsity, convertDenseToCOOupper) {
    using Source      = sp::Dense<config_t>;
    using Result      = sp::SparseCOO<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    Source coo{
        .rows     = 4,
        .cols     = 4,
        .symmetry = sp::Symmetry::Upper,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    indexvec expected_row_idcs(10), expected_col_idcs(10);
    expected_row_idcs << 0, 0, 1, 0, 1, 2, 0, 1, 2, 3;
    expected_col_idcs << 0, 1, 1, 2, 2, 2, 3, 3, 3, 3;
    EXPECT_THAT(result.row_indices, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.col_indices, EigenEqual(expected_col_idcs));
    mat m(4, 4);
    m << 11, 12, 13, 14, 21, 22, 23, 24, 31, 32, 33, 34, 41, 42, 43, 44;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m.reshaped()), v);
    vec expected_v(10);
    expected_v << 11, 12, 22, 13, 23, 33, 14, 24, 34, 44;
    EXPECT_THAT(v, EigenEqual(expected_v));
}

TEST(Sparsity, convertCSCToCOO) {
    using Source      = sp::SparseCSC<config_t, index_t>;
    using Result      = sp::SparseCOO<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_ptrs(5);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_ptrs << 0, 2, 4, 6, 7;
    Source coo{
        .rows      = 3,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Unsymmetric,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::Unsorted,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_idcs(7);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_idcs << 0, 0, 1, 1, 2, 2, 3;
    EXPECT_THAT(result.row_indices, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.col_indices, EigenEqual(expected_col_idcs));
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

TEST(Sparsity, convertCSCToCOOfirstIndex) {
    using Source      = sp::SparseCSC<config_t, index_t>;
    using Result      = sp::SparseCOO<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_ptrs(5);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_ptrs << 0, 2, 4, 6, 7;
    Source coo{
        .rows      = 3,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Unsymmetric,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::Unsorted,
    };
    converter_t converter{Sparsity{coo}, {.first_index = 1}};
    const auto &result = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_idcs(7);
    expected_row_idcs << 1, 2, 2, 3, 1, 3, 1;
    expected_col_idcs << 1, 1, 2, 2, 3, 3, 4;
    EXPECT_THAT(result.row_indices, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.col_indices, EigenEqual(expected_col_idcs));
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

TEST(Sparsity, convertCOOToCOO) {
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::SparseCOO<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_idcs << 0, 0, 1, 1, 2, 2, 3;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::Unsorted,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    EXPECT_THAT(result.row_indices, EigenEqual(row_idcs));
    EXPECT_THAT(result.col_indices, EigenEqual(col_idcs));
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

TEST(Sparsity, convertCOOToCOOfirstIndex) {
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::SparseCOO<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_idcs << 0, 0, 1, 1, 2, 2, 3;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::Unsorted,
    };
    converter_t converter{Sparsity{coo}, {.first_index = 1}};
    const auto &result = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_idcs(7);
    expected_row_idcs << 1, 2, 2, 3, 1, 3, 1;
    expected_col_idcs << 1, 1, 2, 2, 3, 3, 4;
    EXPECT_THAT(result.row_indices, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.col_indices, EigenEqual(expected_col_idcs));
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

TEST(Sparsity, convertCOOToCOOfirstIndex2) {
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::SparseCOO<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 10, 11, 11, 12, 10, 12, 10;
    col_idcs << 10, 10, 11, 11, 12, 12, 13;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::Unsorted,
        .first_index = 10,
    };
    converter_t converter{Sparsity{coo}, {.first_index = 1}};
    const auto &result = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_idcs(7);
    expected_row_idcs << 1, 2, 2, 3, 1, 3, 1;
    expected_col_idcs << 1, 1, 2, 2, 3, 3, 4;
    EXPECT_THAT(result.row_indices, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.col_indices, EigenEqual(expected_col_idcs));
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

TEST(Sparsity, convertCOOintToCOO) {
    using Source      = sp::SparseCOO<config_t, int>;
    using Result      = sp::SparseCOO<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    Eigen::VectorX<int> row_idcs(7), col_idcs(7);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_idcs << 0, 0, 1, 1, 2, 2, 3;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::Unsorted,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_idcs(7);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_idcs << 0, 0, 1, 1, 2, 2, 3;
    EXPECT_THAT(result.row_indices, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.col_indices, EigenEqual(expected_col_idcs));
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

/// @test   sorted CSC to unsorted CSC, so should be no-op
TEST(Sparsity, convertCSCToCSCsorted) {
    using Source      = sp::SparseCSC<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_ptrs(5);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_ptrs << 0, 2, 4, 6, 7;
    Source coo{
        .rows      = 3,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Unsymmetric,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::SortedRows,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_ptrs(5);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_ptrs << 0, 2, 4, 6, 7;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::SortedRows);
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

/// @test   unsorted CSC to unsorted CSC, so should be no-op
TEST(Sparsity, convertCSCToCSCunsorted) {
    using Source      = sp::SparseCSC<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_ptrs(5);
    row_idcs << 1, 0, 2, 1, 2, 0, 0;
    col_ptrs << 0, 2, 4, 6, 7;
    Source coo{
        .rows      = 3,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Unsymmetric,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::Unsorted,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_ptrs(5);
    expected_row_idcs << 1, 0, 2, 1, 2, 0, 0;
    expected_col_ptrs << 0, 2, 4, 6, 7;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::Unsorted);
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

#if ALPAQA_HAVE_COO_CSC_CONVERSIONS
#define SKIP_IF_NO_CSC_CONV() (void)0
#else
#define SKIP_IF_NO_CSC_CONV() GTEST_SKIP()
#endif

/// @test   unsorted CSC to sorted CSC, requires sorting each column
TEST(Sparsity, convertCSCToCSCunsorted2sorted) {
    SKIP_IF_NO_CSC_CONV();
    using Source      = sp::SparseCSC<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_ptrs(5);
    row_idcs << 1, 0, 2, 1, 2, 0, 0;
    col_ptrs << 0, 2, 4, 6, 7;
    Source coo{
        .rows      = 3,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Unsymmetric,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::Unsorted,
    };
    converter_t converter{Sparsity{coo}, {.order = Result::SortedRows}};
    const auto &result = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_ptrs(5);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_ptrs << 0, 2, 4, 6, 7;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::SortedRows);
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    vec expected_v(result.nnz());
    expected_v << 2, 1, 4, 3, 6, 5, 7;
    EXPECT_THAT(v, EigenEqual(expected_v));
}

/// @test   sorted CSC to sorted CSC, should be no-op
TEST(Sparsity, convertCSCToCSCsorted2sorted) {
    using Source      = sp::SparseCSC<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_ptrs(5);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_ptrs << 0, 2, 4, 6, 7;
    Source coo{
        .rows      = 3,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Unsymmetric,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::SortedRows,
    };
    converter_t converter{Sparsity{coo}, {.order = Result::SortedRows}};
    const auto &result = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_ptrs(5);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_ptrs << 0, 2, 4, 6, 7;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::SortedRows);
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

/// @test   sorted CSC int to sorted CSC, requires copy
TEST(Sparsity, convertCSCToCSCsortedInt2sorted) {
    using Source      = sp::SparseCSC<config_t, int>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    Eigen::VectorX<int> row_idcs(7), col_ptrs(5);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_ptrs << 0, 2, 4, 6, 7;
    Source coo{
        .rows      = 3,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Unsymmetric,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::SortedRows,
    };
    converter_t converter{Sparsity{coo}, {.order = Result::SortedRows}};
    const auto &result = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_ptrs(5);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_ptrs << 0, 2, 4, 6, 7;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::SortedRows);
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

/// @test   sorted CSC to sorted CSC int, requires copy
TEST(Sparsity, convertCSCToCSCsorted2sortedInt) {
    using Source      = sp::SparseCSC<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, int>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_ptrs(5);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_ptrs << 0, 2, 4, 6, 7;
    Source coo{
        .rows      = 3,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Unsymmetric,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::SortedRows,
    };
    converter_t converter{Sparsity{coo}, {.order = Result::SortedRows}};
    const auto &result = converter.get_sparsity();
    Eigen::VectorX<int> expected_row_idcs(7), expected_col_ptrs(5);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_ptrs << 0, 2, 4, 6, 7;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::SortedRows);
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

/// @test   unsymmetric Dense to CSC
TEST(Sparsity, convertDenseToCSC) {
    using Source      = sp::Dense<config_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    Source coo{
        .rows     = 3,
        .cols     = 4,
        .symmetry = sp::Symmetry::Unsymmetric,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    indexvec expected_row_idcs(12), expected_col_ptrs(5);
    expected_row_idcs << 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2;
    expected_col_ptrs << 0, 3, 6, 9, 12;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 12);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::SortedRows);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    vec m(12);
    m << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

/// @test   symmetric Dense to CSC
TEST(Sparsity, convertDenseToCSCupper) {
    using Source      = sp::Dense<config_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    Source coo{
        .rows     = 4,
        .cols     = 4,
        .symmetry = sp::Symmetry::Upper,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    indexvec expected_row_idcs(10), expected_col_ptrs(5);
    expected_row_idcs << 0, 0, 1, 0, 1, 2, 0, 1, 2, 3;
    expected_col_ptrs << 0, 1, 3, 6, 10;
    EXPECT_EQ(result.rows, 4);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 10);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    EXPECT_EQ(result.symmetry, sp::Symmetry::Upper);
    EXPECT_EQ(result.order, Result::SortedRows);
    mat m(4, 4);
    m << 11, 12, 13, 14, 21, 22, 23, 24, 31, 32, 33, 34, 41, 42, 43, 44;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m.reshaped()), v);
    vec expected_v(10);
    expected_v << 11, 12, 22, 13, 23, 33, 14, 24, 34, 44;
    EXPECT_THAT(v, EigenEqual(expected_v));
}

/// @test   unsorted COO to unsorted CSC, requires sorting columns
TEST(Sparsity, convertCOOToCSCunsorted) {
    SKIP_IF_NO_CSC_CONV();
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_idcs << 3, 0, 1, 2, 0, 1, 2;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::Unsorted,
    };
    converter_t converter{Sparsity{coo}};
    const auto &result = converter.get_sparsity();
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::Unsorted);
    // The order is indeterminate (since we don't use a stable sort), so
    // compare the matrices using Eigen triplets.
    std::vector<Eigen::Triplet<real_t, index_t>> a_trip, b_trip;
    vec m(7);
    for (index_t i = 0; i < 7; ++i) {
        b_trip.emplace_back(row_idcs(i), col_idcs(i), real_t(i));
        m(i) = real_t(i);
    }
    vec v(7);
    converter.convert_values(cvt_values_func(m), v);
    for (index_t c = 0; c < 4; ++c)
        for (index_t i = result.outer_ptr(c); i < result.outer_ptr(c + 1); ++i)
            a_trip.emplace_back(result.inner_idx(i), c, v(i));
    Eigen::SparseMatrix<real_t, 0, index_t> a(3, 4), b(3, 4);
    a.setFromTriplets(a_trip.begin(), a_trip.end());
    b.setFromTriplets(b_trip.begin(), b_trip.end());
    EXPECT_THAT(a.toDense(), EigenEqual(b.toDense()));
}

/// @test   COO which is sorted but reported as unsorted, to unsorted CSC
TEST(Sparsity, convertCOOToCSCsilentlySorted2unsorted) {
    SKIP_IF_NO_CSC_CONV();
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_idcs << 0, 0, 1, 1, 2, 2, 3;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::Unsorted,
    };
    converter_t converter{Sparsity{coo}};
    const auto &result = converter.get_sparsity();
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::Unsorted);
    // The order is indeterminate (since we don't use a stable sort), so
    // compare the matrices using Eigen triplets.
    std::vector<Eigen::Triplet<real_t, index_t>> a_trip, b_trip;
    vec m(7);
    for (index_t i = 0; i < 7; ++i) {
        b_trip.emplace_back(row_idcs(i), col_idcs(i), real_t(i));
        m(i) = real_t(i);
    }
    vec v(7);
    converter.convert_values(cvt_values_func(m), v);
    for (index_t c = 0; c < 4; ++c)
        for (index_t i = result.outer_ptr(c); i < result.outer_ptr(c + 1); ++i)
            a_trip.emplace_back(result.inner_idx(i), c, v(i));
    Eigen::SparseMatrix<real_t, 0, index_t> a(3, 4), b(3, 4);
    a.setFromTriplets(a_trip.begin(), a_trip.end());
    b.setFromTriplets(b_trip.begin(), b_trip.end());
    EXPECT_THAT(a.toDense(), EigenEqual(b.toDense()));
}

/// @test   COO which is sorted by columns only, to unsorted CSC,
///         no sorting required
TEST(Sparsity, convertCOOToCSCsortedCols2unsorted) {
    SKIP_IF_NO_CSC_CONV();
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 1, 0, 2, 1, 2, 0, 0;
    col_idcs << 0, 0, 1, 1, 2, 2, 3;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::SortedByColsOnly,
    };
    converter_t converter{Sparsity{coo}};
    const auto &result = converter.get_sparsity();
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::Unsorted);
    // The order is indeterminate (since we don't use a stable sort), so
    // compare the matrices using Eigen triplets.
    std::vector<Eigen::Triplet<real_t, index_t>> a_trip, b_trip;
    vec m(7);
    for (index_t i = 0; i < 7; ++i) {
        b_trip.emplace_back(row_idcs(i), col_idcs(i), real_t(i));
        m(i) = real_t(i);
    }
    vec v(7);
    converter.convert_values(cvt_values_func(m), v);
    for (index_t c = 0; c < 4; ++c)
        for (index_t i = result.outer_ptr(c); i < result.outer_ptr(c + 1); ++i)
            a_trip.emplace_back(result.inner_idx(i), c, v(i));
    Eigen::SparseMatrix<real_t, 0, index_t> a(3, 4), b(3, 4);
    a.setFromTriplets(a_trip.begin(), a_trip.end());
    b.setFromTriplets(b_trip.begin(), b_trip.end());
    EXPECT_THAT(a.toDense(), EigenEqual(b.toDense()));
}

/// @test   COO which is sorted but reported as unsorted, to sorted CSC
TEST(Sparsity, convertCOOToCSCsilentlySorted2sorted) {
    SKIP_IF_NO_CSC_CONV();
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_idcs << 0, 0, 1, 1, 2, 2, 3;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::Unsorted,
    };
    converter_t converter{Sparsity{coo}, {.order = Result::SortedRows}};
    const auto &result = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_ptrs(5);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_ptrs << 0, 2, 4, 6, 7;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::SortedRows);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

/// @test   unsorted COO to sorted CSC
TEST(Sparsity, convertCOOToCSCunsorted2sorted) {
    SKIP_IF_NO_CSC_CONV();
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_idcs << 3, 0, 1, 2, 0, 1, 2;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::Unsorted,
    };
    converter_t converter{Sparsity{coo}, {.order = Result::SortedRows}};
    const auto &result = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_ptrs(5);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_ptrs << 0, 2, 4, 6, 7;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::SortedRows);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    vec expected_v(result.nnz());
    expected_v << 5, 2, 3, 6, 7, 4, 1;
    EXPECT_THAT(v, EigenEqual(expected_v));
}

/// @test sorted COO to sorted CSC, no sorting required
TEST(Sparsity, convertCOOToCSCsorted2sorted) {
    SKIP_IF_NO_CSC_CONV();
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_idcs << 0, 0, 1, 1, 2, 2, 3;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::SortedByColsAndRows,
    };
    converter_t converter{Sparsity{coo}, {.order = Result::SortedRows}};
    const auto &result = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_ptrs(5);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_ptrs << 0, 2, 4, 6, 7;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::SortedRows);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

/// @test sorted COO to sorted CSC, with different first index for COO
TEST(Sparsity, convertCOOToCSCfirstIndex) {
    SKIP_IF_NO_CSC_CONV();
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 10, 11, 11, 12, 10, 12, 10;
    col_idcs << 10, 10, 11, 11, 12, 12, 13;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::SortedByColsAndRows,
        .first_index = 10,
    };
    converter_t converter{Sparsity{coo}, {.order = Result::SortedRows}};
    const auto &result = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_ptrs(5);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_ptrs << 0, 2, 4, 6, 7;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::SortedRows);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

/// @test   sorted COO to unsorted CSC, no sorting required
TEST(Sparsity, convertCOOToCSCsorted2unsorted) {
    SKIP_IF_NO_CSC_CONV();
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::SparseCSC<config_t, index_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_idcs << 0, 0, 1, 1, 2, 2, 3;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::SortedByColsAndRows,
    };
    converter_t converter{Sparsity{coo}};
    const auto &result = converter.get_sparsity();
    indexvec expected_row_idcs(7), expected_col_ptrs(5);
    expected_row_idcs << 0, 1, 1, 2, 0, 2, 0;
    expected_col_ptrs << 0, 2, 4, 6, 7;
    EXPECT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(result.nnz(), 7);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    EXPECT_EQ(result.order, Result::SortedRows);
    EXPECT_THAT(result.inner_idx, EigenEqual(expected_row_idcs));
    EXPECT_THAT(result.outer_ptr, EigenEqual(expected_col_ptrs));
    vec m(7);
    m << 1, 2, 3, 4, 5, 6, 7;
    vec v(result.nnz());
    converter.convert_values(cvt_values_func(m), v);
    EXPECT_THAT(v, EigenEqual(m));
}

/// @test   dense to dense, trivial
TEST(Sparsity, convertDenseToDense) {
    using Source      = sp::Dense<config_t>;
    using Result      = sp::Dense<config_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    Source dense{
        .rows     = 3,
        .cols     = 4,
        .symmetry = sp::Symmetry::Unsymmetric,
    };
    converter_t converter = Sparsity{dense};
    const auto &result    = converter.get_sparsity();
    ASSERT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(get_nnz(Sparsity{result}), 12);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    mat m(3, 4);
    m << 11, 12, 13, 14, 21, 22, 23, 24, 31, 32, 33, 34;
    vec v(12);
    converter.convert_values(cvt_values_func(m.reshaped()), v);
    EXPECT_THAT(v, EigenEqual(m.reshaped()));
}

/// @test   CSC to dense
TEST(Sparsity, convertCSCToDense) {
    using Source      = sp::SparseCSC<config_t, index_t>;
    using Result      = sp::Dense<config_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_ptrs(5);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_ptrs << 0, 2, 4, 6, 7;
    Source csc{
        .rows      = 3,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Unsymmetric,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::SortedRows,
    };
    converter_t converter = Sparsity{csc};
    const auto &result    = converter.get_sparsity();
    ASSERT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(get_nnz(Sparsity{result}), 12);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    vec m(7);
    m << 11, 21, 22, 32, 13, 33, 14;
    mat v(3, 4);
    converter.convert_values(cvt_values_func(m), v.reshaped());
    mat expected_v(3, 4);
    expected_v << 11, 0, 13, 14, 21, 22, 0, 0, 0, 32, 33, 0;
    EXPECT_THAT(v, EigenEqual(expected_v));
}

/// @test   CSC upper symmetric to dense
TEST(Sparsity, convertCSCToDenseUpper) {
    using Source      = sp::SparseCSC<config_t, index_t>;
    using Result      = sp::Dense<config_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_ptrs(5);
    row_idcs << 0, 0, 1, 0, 2, 1, 3;
    col_ptrs << 0, 1, 3, 5, 7;
    Source csc{
        .rows      = 4,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Upper,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::SortedRows,
    };
    converter_t converter = Sparsity{csc};
    const auto &result    = converter.get_sparsity();
    ASSERT_EQ(result.rows, 4);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(get_nnz(Sparsity{result}), 16);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Upper);
    vec m(7);
    m << 11, 12, 22, 13, 33, 24, 44;
    mat v(4, 4);
    converter.convert_values(cvt_values_func(m), v.reshaped());
    mat expected_v(4, 4);
    expected_v << 11, 12, 13, 0, 12, 22, 0, 24, 13, 0, 33, 0, 0, 24, 0, 44;
    EXPECT_THAT(v, EigenEqual(expected_v));
}

/// @test   CSC upper symmetric invalid
TEST(Sparsity, convertCSCToDenseUpperInvalid) {
    using Source      = sp::SparseCSC<config_t, index_t>;
    using Result      = sp::Dense<config_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_ptrs(5);
    row_idcs << 0, 0, 2, 0, 2, 1, 3;
    col_ptrs << 0, 1, 3, 5, 7;
    Source csc{
        .rows      = 4,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Upper,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::SortedRows,
    };
    converter_t converter = Sparsity{csc};
    const auto &result    = converter.get_sparsity();
    ASSERT_EQ(result.rows, 4);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(get_nnz(Sparsity{result}), 16);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Upper);
    vec m(7);
    m << 11, 12, 32, 13, 33, 24, 44;
    mat v(4, 4);
    EXPECT_THROW(converter.convert_values(cvt_values_func(m), v.reshaped()),
                 std::invalid_argument);
}

/// @test   CSC lower symmetric to dense
TEST(Sparsity, convertCSCToDenseLower) {
    using Source      = sp::SparseCSC<config_t, index_t>;
    using Result      = sp::Dense<config_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_ptrs(5);
    row_idcs << 0, 1, 2, 1, 3, 2, 3;
    col_ptrs << 0, 3, 5, 6, 7;
    Source csc{
        .rows      = 4,
        .cols      = 4,
        .symmetry  = sp::Symmetry::Lower,
        .inner_idx = row_idcs,
        .outer_ptr = col_ptrs,
        .order     = Source::SortedRows,
    };
    converter_t converter = Sparsity{csc};
    const auto &result    = converter.get_sparsity();
    ASSERT_EQ(result.rows, 4);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(get_nnz(Sparsity{result}), 16);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Lower);
    vec m(7);
    m << 11, 12, 13, 22, 24, 33, 44;
    mat v(4, 4);
    converter.convert_values(cvt_values_func(m), v.reshaped());
    mat expected_v(4, 4);
    expected_v << 11, 12, 13, 0, //
        12, 22, 0, 24,           //
        13, 0, 33, 0,            //
        0, 24, 0, 44;
    EXPECT_THAT(v, EigenEqual(expected_v));
}

/// @test   COO to dense
TEST(Sparsity, convertCOOToDense) {
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::Dense<config_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 1, 1, 2, 0, 2, 0;
    col_idcs << 0, 0, 1, 1, 2, 2, 3;
    Source coo{
        .rows        = 3,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Unsymmetric,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::SortedByColsAndRows,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    ASSERT_EQ(result.rows, 3);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(get_nnz(Sparsity{result}), 12);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Unsymmetric);
    vec m(7);
    m << 11, 21, 22, 32, 13, 33, 14;
    mat v(3, 4);
    converter.convert_values(cvt_values_func(m), v.reshaped());
    mat expected_v(3, 4);
    expected_v << 11, 0, 13, 14, 21, 22, 0, 0, 0, 32, 33, 0;
    EXPECT_THAT(v, EigenEqual(expected_v));
}

/// @test   COO upper symmetric to dense
TEST(Sparsity, convertCOOToDenseUpper) {
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::Dense<config_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 0, 1, 0, 2, 1, 3;
    col_idcs << 0, 1, 1, 2, 2, 3, 3;
    Source coo{
        .rows        = 4,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Upper,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::SortedByColsAndRows,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    ASSERT_EQ(result.rows, 4);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(get_nnz(Sparsity{result}), 16);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Upper);
    vec m(7);
    m << 11, 12, 22, 13, 33, 24, 44;
    mat v(4, 4);
    converter.convert_values(cvt_values_func(m), v.reshaped());
    mat expected_v(4, 4);
    expected_v << 11, 12, 13, 0, 12, 22, 0, 24, 13, 0, 33, 0, 0, 24, 0, 44;
    EXPECT_THAT(v, EigenEqual(expected_v));
}

/// @test   COO upper symmetric invalid
TEST(Sparsity, convertCOOToDenseUpperInvalid) {
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::Dense<config_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 0, 2, 0, 2, 1, 3;
    col_idcs << 0, 1, 1, 2, 2, 3, 3;
    Source coo{
        .rows        = 4,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Upper,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::SortedByColsAndRows,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    ASSERT_EQ(result.rows, 4);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(get_nnz(Sparsity{result}), 16);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Upper);
    vec m(7);
    m << 11, 12, 32, 13, 33, 24, 44;
    mat v(4, 4);
    EXPECT_THROW(converter.convert_values(cvt_values_func(m), v.reshaped()),
                 std::invalid_argument);
}

/// @test   COO lower symmetric to dense
TEST(Sparsity, convertCOOToDenseLower) {
    using Source      = sp::SparseCOO<config_t, index_t>;
    using Result      = sp::Dense<config_t>;
    using Sparsity    = sp::Sparsity<config_t>;
    using converter_t = sp::SparsityConverter<Sparsity, Result>;
    indexvec row_idcs(7), col_idcs(7);
    row_idcs << 0, 1, 2, 1, 3, 2, 3;
    col_idcs << 0, 0, 0, 1, 1, 2, 3;
    Source coo{
        .rows        = 4,
        .cols        = 4,
        .symmetry    = sp::Symmetry::Lower,
        .row_indices = row_idcs,
        .col_indices = col_idcs,
        .order       = Source::SortedByColsAndRows,
    };
    converter_t converter = Sparsity{coo};
    const auto &result    = converter.get_sparsity();
    ASSERT_EQ(result.rows, 4);
    ASSERT_EQ(result.cols, 4);
    ASSERT_EQ(get_nnz(Sparsity{result}), 16);
    EXPECT_EQ(result.symmetry, sp::Symmetry::Lower);
    vec m(7);
    m << 11, 12, 13, 22, 24, 33, 44;
    mat v(4, 4);
    converter.convert_values(cvt_values_func(m), v.reshaped());
    mat expected_v(4, 4);
    expected_v << 11, 12, 13, 0, //
        12, 22, 0, 24,           //
        13, 0, 33, 0,            //
        0, 24, 0, 44;
    EXPECT_THAT(v, EigenEqual(expected_v));
}