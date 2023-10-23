#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/export.hpp>
#include <cassert>
#include <concepts>
#include <variant>

namespace alpaqa::sparsity {

/// Describes the symmetry of matrices.
enum class Symmetry {
    Unsymmetric = 0, ///< No symmetry.
    Upper       = 1, ///< Symmetric, upper-triangular part is stored.
    Lower       = 2, ///< Symmetric, lower-triangular part is stored.
};

/// Dense matrix structure. Stores all elements in column-major storage.
/// Symmetric dense matrices always store all elements.
template <Config Conf>
struct Dense {
    USING_ALPAQA_CONFIG(Conf);
    length_t rows = 0, cols = 0;
    Symmetry symmetry = Symmetry::Unsymmetric;
};

/// Sparse compressed-column structure (CCS or CSC).
template <Config Conf, class StorageIndex>
struct SparseCSC {
    USING_ALPAQA_CONFIG(Conf);
    using storage_index_t     = StorageIndex;
    using index_vector_t      = Eigen::VectorX<storage_index_t>;
    using index_vector_view_t = Eigen::Ref<const index_vector_t>;
    using index_vector_map_t  = Eigen::Map<const index_vector_t>;
    length_t rows = 0, cols = 0;
    Symmetry symmetry             = Symmetry::Unsymmetric;
    index_vector_view_t inner_idx = index_vector_map_t{nullptr, 0};
    index_vector_view_t outer_ptr = index_vector_map_t{nullptr, 0};
    enum Order {
        /// The row indices are not sorted.
        Unsorted = 0,
        /// Within each column, all row indices are sorted in ascending order.
        SortedRows = 1,
    };
    Order order = Unsorted;

    /// Get the number of structurally nonzero elements.
    length_t nnz() const {
        assert(outer_ptr.size() == cols + 1);
        return inner_idx.size();
    }
};

/// Sparse coordinate list structure (COO).
template <Config Conf, class StorageIndex>
struct SparseCOO {
    USING_ALPAQA_CONFIG(Conf);
    using storage_index_t     = StorageIndex;
    using index_vector_t      = Eigen::VectorX<storage_index_t>;
    using index_vector_view_t = Eigen::Ref<const index_vector_t>;
    using index_vector_map_t  = Eigen::Map<const index_vector_t>;
    length_t rows = 0, cols = 0;
    Symmetry symmetry               = Symmetry::Unsymmetric;
    index_vector_view_t row_indices = index_vector_map_t{nullptr, 0};
    index_vector_view_t col_indices = index_vector_map_t{nullptr, 0};
    enum Order {
        /// The indices are not sorted.
        Unsorted = 0,
        /// The indices are sorted by column first, and within each column, the
        /// rows are sorted as well.
        SortedByColsAndRows = 1,
        /// The indices are sorted by column, but the rows within each column
        /// are not sorted.
        SortedByColsOnly = 2,
        /// The indices are sorted by row first, and within each row, the
        /// columns are sorted as well.
        SortedByRowsAndCols = 3,
        /// The indices are sorted by row, but the columns within each row are
        /// not sorted.
        SortedByRowsOnly = 4,
    };
    Order order                 = Unsorted;
    storage_index_t first_index = 0; ///< Zero for C/C++, one for Fortran.

    /// Get the number of structurally nonzero elements.
    length_t nnz() const {
        assert(row_indices.size() == col_indices.size());
        return row_indices.size();
    }
};

template <Config Conf>
using SparsityVariant = std::variant< //
    Dense<Conf>,                      //
    SparseCSC<Conf, int>,             //
    SparseCSC<Conf, long>,            //
    SparseCSC<Conf, long long>,       //
    SparseCOO<Conf, int>,             //
    SparseCOO<Conf, long>,            //
    SparseCOO<Conf, long long>        //
    >;

/// Stores any of the supported sparsity patterns.
/// @see @ref SparsityConverter<Sparsity<Conf>, To>
template <Config Conf>
struct Sparsity {
    Sparsity(std::convertible_to<SparsityVariant<Conf>> auto value) : value{std::move(value)} {}
    SparsityVariant<Conf> value;
};

namespace detail {
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
} // namespace detail

/// Returns true if the sparsity pattern represents a dense matrix.
template <Config Conf>
bool is_dense(const Sparsity<Conf> &sp) {
    auto visitor = detail::overloaded{
        [](const Dense<Conf> &) { return true; },
        [](const auto &) { return false; },
    };
    return std::visit(visitor, sp.value);
}

/// Get the number of structurally nonzero elements.
template <Config Conf>
length_t<Conf> get_nnz(const Sparsity<Conf> &sp) {
    auto visitor = detail::overloaded{
        [](const Dense<Conf> &d) { return d.rows * d.cols; },
        [](const auto &s) { return s.nnz(); },
    };
    return std::visit(visitor, sp.value);
}

/// Returns the symmetry of the sparsity pattern.
template <Config Conf>
Symmetry get_symmetry(const Sparsity<Conf> &sp) {
    return std::visit([](const auto &s) { return s.symmetry; }, sp.value);
}

} // namespace alpaqa::sparsity

namespace alpaqa {
using sparsity::Sparsity;
} // namespace alpaqa
