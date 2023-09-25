#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/export.hpp>
#include <cassert>
#include <variant>

namespace alpaqa::sparsity {

enum class Symmetry {
    Unsymmetric = 0,
    Upper = 1,
    Lower = 2,
};

template <Config Conf>
struct Dense {
    USING_ALPAQA_CONFIG(Conf);
    length_t rows = 0, cols = 0;
    Symmetry symmetry = Symmetry::Unsymmetric;
};

template <Config Conf>
struct SparseCSC {
    USING_ALPAQA_CONFIG(Conf);
    length_t rows = 0, cols = 0;
    Symmetry symmetry    = Symmetry::Unsymmetric;
    crindexvec inner_idx = null_indexvec<config_t>;
    crindexvec outer_ptr = null_indexvec<config_t>;
    enum Order {
        /// The row indices are not sorted.
        Unsorted = 0,
        /// Within each column, all row indices are sorted in ascending order.
        SortedRows = 1,
    };
    Order order = Unsorted;

    length_t nnz() const {
        assert(outer_ptr.size() == cols + 1);
        return inner_idx.size();
    }
};

template <Config Conf, class StorageIndex = index_t<Conf>>
struct SparseCOO {
    USING_ALPAQA_CONFIG(Conf);
    using storage_index_t     = StorageIndex;
    using index_vector_t      = Eigen::VectorX<storage_index_t>;
    using index_vector_view_t = Eigen::Ref<const index_vector_t>;
    length_t rows = 0, cols = 0;
    Symmetry symmetry               = Symmetry::Unsymmetric;
    index_vector_view_t row_indices = Eigen::Map<index_vector_t>(nullptr, 0);
    index_vector_view_t col_indices = Eigen::Map<index_vector_t>(nullptr, 0);
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

    length_t nnz() const {
        assert(row_indices.size() == col_indices.size());
        return row_indices.size();
    }
};

template <Config Conf>
using SparsityVariant = std::variant<Dense<Conf>,     //
                                     SparseCSC<Conf>, //
                                     SparseCOO<Conf>, //
                                     SparseCOO<Conf, int>>;

template <Config Conf>
struct Sparsity : SparsityVariant<Conf> {
    using SparsityVariant<Conf>::variant;
};

namespace detail {
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
} // namespace detail

template <Config Conf>
bool is_dense(const Sparsity<Conf> &sp) {
    auto visitor = detail::overloaded{
        [](const Dense<Conf> &) { return true; },
        [](const auto &) { return false; },
    };
    return std::visit(visitor, sp);
}

template <Config Conf>
length_t<Conf> get_nnz(const Sparsity<Conf> &sp) {
    auto visitor = detail::overloaded{
        [](const Dense<Conf> &d) { return d.rows * d.cols; },
        [](const auto &s) { return s.nnz(); },
    };
    return std::visit(visitor, sp);
}

template <Config Conf>
Symmetry get_symmetry(const Sparsity<Conf> &sp) {
    return std::visit([](const auto &s) { return s.symmetry; }, sp);
}

} // namespace alpaqa::sparsity

namespace alpaqa {
using sparsity::Sparsity;
} // namespace alpaqa
