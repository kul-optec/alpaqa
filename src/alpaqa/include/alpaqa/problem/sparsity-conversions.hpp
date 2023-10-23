#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/sparsity.hpp>
#include <alpaqa/util/sparse-ops.hpp>

#include <algorithm>
#include <cassert>
#include <concepts>
#include <numeric>
#include <optional>
#include <stdexcept>

namespace alpaqa::sparsity {

/// Converts one matrix storage format to another.
/// @tparam From
///         The input sparsity pattern type.
/// @tparam To
///         The output sparsity pattern type.
template <class From, class To>
struct SparsityConverter;

/// Additional options for the conversion performed by @ref SparsityConverter.
template <class To>
struct SparsityConversionRequest;

template <Config Conf>
struct SparsityConversionRequest<Dense<Conf>> {};

template <Config Conf>
struct SparsityConverter<Dense<Conf>, Dense<Conf>> {
    USING_ALPAQA_CONFIG(Conf);
    using from_sparsity_t = Dense<Conf>;
    using to_sparsity_t   = Dense<Conf>;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    SparsityConverter(from_sparsity_t from, Request = {}) : sparsity(from) {
        if (from.symmetry != Symmetry::Unsymmetric && from.rows != from.cols)
            throw std::invalid_argument("Nonsquare matrix cannot be symmetric");
    }
    to_sparsity_t sparsity;
    operator const to_sparsity_t &() const & { return sparsity; }
    const to_sparsity_t &get_sparsity() const { return *this; }
    template <class F>
    void convert_values(const F &from, rvec to) const {
        from(to);
    }
};

template <Config Conf, class StorageIndex>
struct SparsityConverter<SparseCSC<Conf, StorageIndex>, Dense<Conf>> {
    USING_ALPAQA_CONFIG(Conf);
    using from_sparsity_t = SparseCSC<Conf, StorageIndex>;
    using to_sparsity_t   = Dense<Conf>;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity(from_sparsity_t from, Request) {
#if ALPAQA_HAVE_COO_CSC_CONVERSIONS
        assert(util::check_uniqueness_csc(from.outer_ptr, from.inner_idx));
#endif
        if (from.symmetry != Symmetry::Unsymmetric && from.rows != from.cols)
            throw std::invalid_argument("Nonsquare matrix cannot be symmetric");
        return {
            .rows     = from.rows,
            .cols     = from.cols,
            .symmetry = from.symmetry,
        };
    }
    SparsityConverter(from_sparsity_t from, Request request = {})
        : from_sparsity(from), sparsity(convert_sparsity(from, request)),
          work(from_sparsity.nnz()) {}
    from_sparsity_t from_sparsity;
    to_sparsity_t sparsity;
    mutable vec work;
    operator const to_sparsity_t &() const & { return sparsity; }
    const to_sparsity_t &get_sparsity() const { return *this; }
    template <class F>
    void convert_values(const F &from, rvec to) const {
        from(work);
        to.setZero();
        auto &&T  = to.reshaped(sparsity.rows, sparsity.cols);
        index_t l = 0;
        for (index_t c = 0; c < from_sparsity.cols; ++c) {
            auto inner_start = from_sparsity.outer_ptr(c);
            auto inner_end   = from_sparsity.outer_ptr(c + 1);
            for (auto i = inner_start; i < inner_end; ++i) {
                auto r = from_sparsity.inner_idx(i);
                switch (from_sparsity.symmetry) {
                    case Symmetry::Unsymmetric: T(r, c) = work(l); break;
                    case Symmetry::Upper:
                        if (r > c)
                            throw std::invalid_argument(
                                "Invalid symmetric CSC matrix: upper-triangular matrix should not "
                                "have elements below the diagonal");
                        T(c, r) = T(r, c) = work(l);
                        break;
                    case Symmetry::Lower:
                        if (r < c)
                            throw std::invalid_argument(
                                "Invalid symmetric CSC matrix: lower-triangular matrix should not "
                                "have elements above the diagonal");
                        T(c, r) = T(r, c) = work(l);
                        break;
                    default: throw std::invalid_argument("Invalid symmetry");
                }
                ++l;
            }
        }
    }
};

template <Config Conf, class StorageIndex>
struct SparsityConverter<SparseCOO<Conf, StorageIndex>, Dense<Conf>> {
    USING_ALPAQA_CONFIG(Conf);
    using from_sparsity_t = SparseCOO<Conf, StorageIndex>;
    using to_sparsity_t   = Dense<Conf>;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity(from_sparsity_t from, Request) {
#if ALPAQA_HAVE_COO_CSC_CONVERSIONS
        assert(util::check_uniqueness_triplets(from.row_indices, from.col_indices));
#endif
        if (from.symmetry != Symmetry::Unsymmetric && from.rows != from.cols)
            throw std::invalid_argument("Nonsquare matrix cannot be symmetric");
        return {
            .rows     = from.rows,
            .cols     = from.cols,
            .symmetry = from.symmetry,
        };
    }
    SparsityConverter(from_sparsity_t from, Request request = {})
        : from_sparsity(from), sparsity(convert_sparsity(from, request)),
          work(from_sparsity.nnz()) {}
    from_sparsity_t from_sparsity;
    to_sparsity_t sparsity;
    mutable vec work;
    operator const to_sparsity_t &() const & { return sparsity; }
    const to_sparsity_t &get_sparsity() const { return *this; }
    template <class F>
    void convert_values(const F &from, rvec to) const {
        from(work);
        to.setZero();
        auto &&T = to.reshaped(sparsity.rows, sparsity.cols);
        for (index_t l = 0; l < from_sparsity.nnz(); ++l) {
            auto r = from_sparsity.row_indices(l) - from_sparsity.first_index;
            auto c = from_sparsity.col_indices(l) - from_sparsity.first_index;
            switch (from_sparsity.symmetry) {
                case Symmetry::Unsymmetric: T(r, c) = work(l); break;
                case Symmetry::Upper:
                    if (r > c)
                        throw std::invalid_argument(
                            "Invalid symmetric COO matrix: upper-triangular matrix should not "
                            "have elements below the diagonal");
                    T(c, r) = T(r, c) = work(l);
                    break;
                case Symmetry::Lower:
                    if (r < c)
                        throw std::invalid_argument(
                            "Invalid symmetric COO matrix: lower-triangular matrix should not "
                            "have elements above the diagonal");
                    T(c, r) = T(r, c) = work(l);
                    break;
                default: throw std::invalid_argument("Invalid symmetry");
            }
        }
    }
};

template <Config Conf, class StorageIndex>
struct SparsityConversionRequest<SparseCOO<Conf, StorageIndex>> {
    /// Convert the index offset (zero for C/C++, one for Fortran).
    std::optional<StorageIndex> first_index = std::nullopt;
};

template <Config Conf, class StorageIndex>
struct SparsityConverter<Dense<Conf>, SparseCOO<Conf, StorageIndex>> {
    USING_ALPAQA_CONFIG(Conf);
    using from_sparsity_t = Dense<Conf>;
    using to_sparsity_t   = SparseCOO<Conf, StorageIndex>;
    using storage_index_t = typename to_sparsity_t::storage_index_t;
    using index_vector_t  = typename to_sparsity_t::index_vector_t;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity(from_sparsity_t from, Request request) {
        storage_index_t Δ = 0;
        if (request.first_index)
            Δ = *request.first_index;
        switch (from.symmetry) {
            case Symmetry::Unsymmetric: {
                length_t nnz = from.rows * from.cols;
                row_indices.resize(nnz);
                col_indices.resize(nnz);
                index_t l = 0;
                for (index_t c = 0; c < from.cols; ++c) {
                    for (index_t r = 0; r < from.rows; ++r) {
                        row_indices[l] = static_cast<storage_index_t>(r) + Δ;
                        col_indices[l] = static_cast<storage_index_t>(c) + Δ;
                        ++l;
                    }
                }
            } break;
            case Symmetry::Upper: {
                if (from.rows != from.cols)
                    throw std::invalid_argument("Nonsquare matrix cannot be symmetric");
                length_t nnz = from.rows * (from.rows + 1) / 2;
                row_indices.resize(nnz);
                col_indices.resize(nnz);
                index_t l = 0;
                for (index_t c = 0; c < from.cols; ++c) {
                    for (index_t r = 0; r <= c; ++r) {
                        row_indices[l] = static_cast<storage_index_t>(r) + Δ;
                        col_indices[l] = static_cast<storage_index_t>(c) + Δ;
                        ++l;
                    }
                }
            } break;
            case Symmetry::Lower:
                throw std::invalid_argument("Lower-triangular symmetry currently not supported");
            default: throw std::invalid_argument("Invalid symmetry");
        }
        return {
            .rows        = from.rows,
            .cols        = from.cols,
            .symmetry    = from.symmetry,
            .row_indices = row_indices,
            .col_indices = col_indices,
            .order       = to_sparsity_t::SortedByColsAndRows,
            .first_index = request.first_index ? *request.first_index : 0,
        };
    }
    SparsityConverter(from_sparsity_t from, Request request = {})
        : sparsity(convert_sparsity(from, request)) {
        if (sparsity.symmetry != Symmetry::Unsymmetric)
            work.resize(sparsity.rows * sparsity.cols);
    }
    index_vector_t row_indices, col_indices;
    to_sparsity_t sparsity;
    mutable vec work;
    operator const to_sparsity_t &() const & { return sparsity; }
    const to_sparsity_t &get_sparsity() const { return *this; }
    template <class F>
    void convert_values(const F &from, rvec to) const {
        if (sparsity.symmetry == Symmetry::Unsymmetric) {
            from(to);
        } else if (sparsity.symmetry == Symmetry::Upper) {
            from(work);
            auto &&f = work.reshaped(sparsity.rows, sparsity.cols);
            auto t   = to.begin();
            for (index_t c = 0; c < sparsity.cols; ++c)
                std::ranges::copy_backward(f.col(c).topRows(c + 1), t += c + 1);
        }
    }
};

template <Config Conf, class StorageIndexFrom, class StorageIndexTo>
struct SparsityConverter<SparseCSC<Conf, StorageIndexFrom>, SparseCOO<Conf, StorageIndexTo>> {
    USING_ALPAQA_CONFIG(Conf);
    using from_sparsity_t = SparseCSC<Conf, StorageIndexFrom>;
    using to_sparsity_t   = SparseCOO<Conf, StorageIndexTo>;
    using storage_index_t = typename to_sparsity_t::storage_index_t;
    using index_vector_t  = typename to_sparsity_t::index_vector_t;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity(from_sparsity_t from, Request request) {
        storage_index_t Δ = 0;
        if (request.first_index)
            Δ = *request.first_index;
        row_indices.resize(from.nnz());
        col_indices.resize(from.nnz());
        index_t l = 0;
        for (index_t c = 0; c < from.cols; ++c) {
            auto inner_start = from.outer_ptr(c);
            auto inner_end   = from.outer_ptr(c + 1);
            for (auto i = inner_start; i < inner_end; ++i) {
                auto r         = from.inner_idx(i);
                row_indices[l] = static_cast<storage_index_t>(r) + Δ;
                col_indices[l] = static_cast<storage_index_t>(c) + Δ;
                ++l;
            }
        }
        return {
            .rows        = from.rows,
            .cols        = from.cols,
            .symmetry    = from.symmetry,
            .row_indices = row_indices,
            .col_indices = col_indices,
            .order = from.order == from_sparsity_t::SortedRows ? to_sparsity_t::SortedByColsAndRows
                                                               : to_sparsity_t::SortedByColsOnly,
            .first_index = request.first_index ? *request.first_index : 0,
        };
    }
    SparsityConverter(from_sparsity_t from, Request request = {})
        : sparsity(convert_sparsity(from, request)) {
#if ALPAQA_HAVE_COO_CSC_CONVERSIONS
        assert(util::check_uniqueness_triplets(sparsity.row_indices, sparsity.col_indices));
#endif
    }
    index_vector_t row_indices, col_indices;
    to_sparsity_t sparsity;
    operator const to_sparsity_t &() const & { return sparsity; }
    const to_sparsity_t &get_sparsity() const { return *this; }
    template <class F>
    void convert_values(const F &from, rvec to) const {
        from(to);
    }
};

template <Config Conf, class StorageIndexFrom, class StorageIndexTo>
struct SparsityConverter<SparseCOO<Conf, StorageIndexFrom>, SparseCOO<Conf, StorageIndexTo>> {
    USING_ALPAQA_CONFIG(Conf);
    using from_sparsity_t = SparseCOO<Conf, StorageIndexFrom>;
    using to_sparsity_t   = SparseCOO<Conf, StorageIndexTo>;
    using storage_index_t = typename to_sparsity_t::storage_index_t;
    using index_vector_t  = typename to_sparsity_t::index_vector_t;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity(from_sparsity_t from, Request request) {
        storage_index_t Δ = 0;
        if (request.first_index)
            Δ = *request.first_index - static_cast<storage_index_t>(from.first_index);
        // Check if we can fully reuse the indices without changes
        if constexpr (std::is_same_v<StorageIndexFrom, StorageIndexTo>)
            if (Δ == 0)
                return from;
        // Otherwise, allocate space for shifted or converted indices
        row_indices.resize(from.nnz());
        col_indices.resize(from.nnz());
        auto cvt_idx = [Δ](auto i) { return static_cast<storage_index_t>(i) + Δ; };
        std::ranges::transform(from.row_indices, row_indices.begin(), cvt_idx);
        std::ranges::transform(from.col_indices, col_indices.begin(), cvt_idx);
        return {
            .rows        = from.rows,
            .cols        = from.cols,
            .symmetry    = from.symmetry,
            .row_indices = row_indices,
            .col_indices = col_indices,
            .order       = static_cast<typename to_sparsity_t::Order>(from.order),
            .first_index = request.first_index ? *request.first_index
                                               : static_cast<storage_index_t>(from.first_index),
        };
    }
    SparsityConverter(from_sparsity_t from, Request request = {})
        : sparsity(convert_sparsity(from, request)) {
#if ALPAQA_HAVE_COO_CSC_CONVERSIONS
        assert(util::check_uniqueness_triplets(sparsity.row_indices, sparsity.col_indices));
#endif
    }
    index_vector_t row_indices, col_indices;
    to_sparsity_t sparsity;
    operator const to_sparsity_t &() const & { return sparsity; }
    const to_sparsity_t &get_sparsity() const { return *this; }
    template <class F>
    void convert_values(const F &from, rvec to) const {
        from(to);
    }
};

template <Config Conf, class StorageIndex>
struct SparsityConversionRequest<SparseCSC<Conf, StorageIndex>> {
    /// Sort the indices.
    std::optional<typename SparseCSC<Conf, StorageIndex>::Order> order = std::nullopt;
};

template <Config Conf, class StorageIndexFrom, class StorageIndexFromTo>
struct SparsityConverter<SparseCOO<Conf, StorageIndexFrom>, SparseCSC<Conf, StorageIndexFromTo>> {
    USING_ALPAQA_CONFIG(Conf);
    using to_sparsity_t   = SparseCSC<Conf, StorageIndexFromTo>;
    using from_sparsity_t = SparseCOO<Conf, StorageIndexFrom>;
    using storage_index_t = typename to_sparsity_t::storage_index_t;
    using index_vector_t  = typename to_sparsity_t::index_vector_t;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity([[maybe_unused]] from_sparsity_t from,
                                   [[maybe_unused]] Request request) {
#if ALPAQA_HAVE_COO_CSC_CONVERSIONS
        // Convert the indices
        // TODO: using indexvec here could be suboptimal if the original storage
        //       index was int
        index_vector_t row_indices(from.nnz()), col_indices(from.nnz());
        auto cvt_idx = [](auto i) { return static_cast<storage_index_t>(i); };
        std::ranges::transform(from.row_indices, row_indices.begin(), cvt_idx);
        std::ranges::transform(from.col_indices, col_indices.begin(), cvt_idx);
        // Sort the indices
        typename to_sparsity_t::Order order;
        if (request.order && *request.order == to_sparsity_t::SortedRows) {
            order = to_sparsity_t::SortedRows;
            switch (from.order) {
                case from_sparsity_t::SortedByColsAndRows:
                    // No sorting necessary
                    break;
                case from_sparsity_t::Unsorted: [[fallthrough]];
                case from_sparsity_t::SortedByColsOnly: [[fallthrough]];
                case from_sparsity_t::SortedByRowsAndCols: [[fallthrough]];
                case from_sparsity_t::SortedByRowsOnly:
                    permutation.resize(from.nnz());
                    std::iota(permutation.begin(), permutation.end(), index_t{0});
                    util::sort_triplets(row_indices, col_indices, permutation);
                    break;
                default: throw std::invalid_argument("Invalid order");
            }
        } else {
            switch (from.order) {
                case from_sparsity_t::SortedByColsAndRows:
                    order = to_sparsity_t::SortedRows;
                    // No sorting necessary
                    break;
                case from_sparsity_t::SortedByColsOnly:
                    order = to_sparsity_t::Unsorted;
                    // No sorting necessary
                    break;
                case from_sparsity_t::Unsorted: [[fallthrough]];
                case from_sparsity_t::SortedByRowsAndCols: [[fallthrough]];
                case from_sparsity_t::SortedByRowsOnly:
                    order = to_sparsity_t::Unsorted;
                    permutation.resize(from.nnz());
                    std::iota(permutation.begin(), permutation.end(), index_t{0});
                    util::sort_triplets_col(row_indices, col_indices, permutation);
                    break;
                default: throw std::invalid_argument("Invalid order");
            }
        }
        assert(!request.order || *request.order == order);
        if (std::ranges::is_sorted(permutation))
            permutation = indexvec{};
        // Convert the sorted COO format to CSC
        inner_idx.resize(from.nnz());
        outer_ptr.resize(from.cols + 1);
        util::convert_triplets_to_ccs(row_indices, col_indices, inner_idx, outer_ptr,
                                      cvt_idx(from.first_index));
        return {
            .rows      = from.rows,
            .cols      = from.cols,
            .symmetry  = from.symmetry,
            .inner_idx = inner_idx,
            .outer_ptr = outer_ptr,
            .order     = order,
        };
#else
        throw std::runtime_error(
            "This build of alpaqa does not support conversions from sparse COO format to CSC "
            "format. Please recompile with a C++23-compliant compiler.");
#endif
    }
    SparsityConverter(from_sparsity_t from, Request request = {})
        : sparsity(convert_sparsity(from, request)) {
#if ALPAQA_HAVE_COO_CSC_CONVERSIONS
        assert(util::check_uniqueness_csc(sparsity.outer_ptr, sparsity.inner_idx));
#endif
        if (permutation.size() > 0)
            work.resize(sparsity.nnz());
    }
    index_vector_t inner_idx, outer_ptr;
    indexvec permutation;
    to_sparsity_t sparsity;
    mutable vec work;
    operator const to_sparsity_t &() const & { return sparsity; }
    const to_sparsity_t &get_sparsity() const { return *this; }
    template <class F>
    void convert_values(const F &from, rvec to) const {
        if (permutation.size() > 0) {
            from(work);
            to = work(permutation);
        } else {
            from(to);
        }
    }
};

template <Config Conf, class StorageIndexFrom, class StorageIndexTo>
struct SparsityConverter<SparseCSC<Conf, StorageIndexFrom>, SparseCSC<Conf, StorageIndexTo>> {
    USING_ALPAQA_CONFIG(Conf);
    using to_sparsity_t       = SparseCSC<Conf, StorageIndexTo>;
    using from_sparsity_t     = SparseCSC<Conf, StorageIndexFrom>;
    using storage_index_t     = typename to_sparsity_t::storage_index_t;
    using index_vector_t      = typename to_sparsity_t::index_vector_t;
    using index_vector_view_t = typename to_sparsity_t::index_vector_view_t;
    using Request             = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity([[maybe_unused]] from_sparsity_t from, Request request) {
        auto cvt_idx         = [](auto i) { return static_cast<storage_index_t>(i); };
        auto convert_indices = [&] {
            inner_idx.resize(from.inner_idx.size());
            std::ranges::transform(from.inner_idx, inner_idx.begin(), cvt_idx);
            outer_ptr.resize(from.outer_ptr.size());
            std::ranges::transform(from.outer_ptr, outer_ptr.begin(), cvt_idx);
        };
        auto sort_indices = [&] {
#if ALPAQA_HAVE_COO_CSC_CONVERSIONS
            convert_indices();
            permutation.resize(from.nnz());
            std::iota(permutation.begin(), permutation.end(), index_t{0});
            util::sort_rows_csc(outer_ptr, inner_idx, permutation);
#else
            throw std::runtime_error(
                "This build of alpaqa does not support sorting matrices in CSC format. "
                "Please recompile with a C++23-compliant compiler.");
#endif
        };
        using Order       = typename to_sparsity_t::Order;
        bool need_sorting = false;
        auto order        = static_cast<Order>(from.order);
        // Check whether the user requested the indices to be sorted
        if (request.order && *request.order == to_sparsity_t::SortedRows) {
            order = to_sparsity_t::SortedRows;
            switch (from.order) {
                case from_sparsity_t::Unsorted: need_sorting = true; break;
                case from_sparsity_t::SortedRows: need_sorting = false; break;
                default: throw std::invalid_argument("Invalid order");
            }
        }
        // Convert and sort, or only convert the indices
        if (need_sorting)
            sort_indices();
        else if (!std::is_same_v<StorageIndexFrom, StorageIndexTo>)
            convert_indices();
        // Remove unnecessary permutations
        if (std::ranges::is_sorted(permutation))
            permutation = indexvec{};
        // If the index types are the same, we may be able to reuse the storage
        if constexpr (std::is_same_v<StorageIndexFrom, StorageIndexTo>)
            return {
                .rows      = from.rows,
                .cols      = from.cols,
                .symmetry  = from.symmetry,
                .inner_idx = need_sorting ? index_vector_view_t{inner_idx} : from.inner_idx,
                .outer_ptr = need_sorting ? index_vector_view_t{outer_ptr} : from.outer_ptr,
                .order     = order,
            };
        // If the index types are not the same, we have to copy regardless
        else
            return {
                .rows      = from.rows,
                .cols      = from.cols,
                .symmetry  = from.symmetry,
                .inner_idx = inner_idx,
                .outer_ptr = outer_ptr,
                .order     = order,
            };
    }
    SparsityConverter(from_sparsity_t from, Request request = {})
        : sparsity(convert_sparsity(from, request)) {
#if ALPAQA_HAVE_COO_CSC_CONVERSIONS
        assert(util::check_uniqueness_csc(sparsity.outer_ptr, sparsity.inner_idx));
#endif
        if (permutation.size() > 0)
            work.resize(sparsity.nnz());
    }
    index_vector_t inner_idx, outer_ptr;
    indexvec permutation;
    to_sparsity_t sparsity;
    mutable vec work;
    operator const to_sparsity_t &() const & { return sparsity; }
    const to_sparsity_t &get_sparsity() const { return *this; }
    template <class F>
    void convert_values(const F &from, rvec to) const {
        if (permutation.size() > 0) {
            from(work);
            to = work(permutation);
        } else {
            from(to);
        }
    }
};

template <Config Conf, class StorageIndex>
struct SparsityConverter<Dense<Conf>, SparseCSC<Conf, StorageIndex>> {
    USING_ALPAQA_CONFIG(Conf);
    using to_sparsity_t       = SparseCSC<Conf, StorageIndex>;
    using from_sparsity_t     = Dense<Conf>;
    using storage_index_t     = typename to_sparsity_t::storage_index_t;
    using index_vector_t      = typename to_sparsity_t::index_vector_t;
    using index_vector_view_t = typename to_sparsity_t::index_vector_view_t;
    using Request             = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity([[maybe_unused]] from_sparsity_t from, Request) {
        auto cvt_idx = [](auto i) { return static_cast<storage_index_t>(i); };
        switch (from.symmetry) {
            case Symmetry::Unsymmetric: {
                inner_idx.resize(from.rows * from.cols);
                outer_ptr.resize(from.cols + 1);
                index_t l = 0;
                for (index_t c = 0; c < from.cols; ++c) {
                    outer_ptr[c] = cvt_idx(l);
                    for (index_t r = 0; r < from.rows; ++r) {
                        inner_idx[l] = cvt_idx(r);
                        ++l;
                    }
                }
                outer_ptr[from.cols] = cvt_idx(l);
            } break;
            case Symmetry::Upper: {
                if (from.rows != from.cols)
                    throw std::invalid_argument("Nonsquare matrix cannot be symmetric");
                inner_idx.resize(from.rows * (from.rows + 1) / 2);
                outer_ptr.resize(from.cols + 1);
                index_t l = 0;
                for (index_t c = 0; c < from.cols; ++c) {
                    outer_ptr[c] = cvt_idx(l);
                    for (index_t r = 0; r <= c; ++r) {
                        inner_idx[l] = cvt_idx(r);
                        ++l;
                    }
                }
                outer_ptr[from.cols] = cvt_idx(l);
            } break;
            case Symmetry::Lower:
                throw std::invalid_argument("Lower-triangular symmetry currently not supported");
            default: throw std::invalid_argument("Invalid symmetry");
        }
        return {
            .rows      = from.rows,
            .cols      = from.cols,
            .symmetry  = from.symmetry,
            .inner_idx = inner_idx,
            .outer_ptr = outer_ptr,
            .order     = to_sparsity_t::SortedRows,
        };
    }
    SparsityConverter(from_sparsity_t from, Request request = {})
        : sparsity(convert_sparsity(from, request)) {
        if (sparsity.symmetry != Symmetry::Unsymmetric)
            work.resize(sparsity.rows * sparsity.cols);
    }
    index_vector_t inner_idx, outer_ptr;
    to_sparsity_t sparsity;
    mutable vec work;
    operator const to_sparsity_t &() const & { return sparsity; }
    const to_sparsity_t &get_sparsity() const { return *this; }
    template <class F>
    void convert_values(const F &from, rvec to) const {
        if (sparsity.symmetry == Symmetry::Unsymmetric) {
            from(to);
        } else if (sparsity.symmetry == Symmetry::Upper) {
            from(work);
            auto &&f = work.reshaped(sparsity.rows, sparsity.cols);
            auto t   = to.begin();
            for (index_t c = 0; c < sparsity.cols; ++c)
                std::ranges::copy_backward(f.col(c).topRows(c + 1), t += c + 1);
        }
    }
};

namespace detail {
template <class To, class>
struct ConverterVariantHelper;

template <class To, class... Froms>
struct ConverterVariantHelper<To, std::variant<Froms...>> {
    using type = std::variant<SparsityConverter<Froms, To>...>;
};
} // namespace detail

template <class To>
using ConverterVariant =
    detail::ConverterVariantHelper<To, SparsityVariant<typename To::config_t>>::type;

/// Converts any supported matrix storage format to the given format.
/// @see @ref Sparsity
template <class Conf, class To>
    requires std::same_as<Conf, typename To::config_t>
struct SparsityConverter<Sparsity<Conf>, To> {
    USING_ALPAQA_CONFIG(Conf);
    using from_sparsity_t = Sparsity<Conf>;
    using to_sparsity_t   = To;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    SparsityConverter(Sparsity<config_t> from, Request request = {})
        : converter{std::visit(wrap_converter(request), from.value)} {}
    ConverterVariant<To> converter;
    operator const to_sparsity_t &() const {
        return std::visit([](const auto &c) -> const to_sparsity_t & { return c; }, converter);
    }
    const to_sparsity_t &get_sparsity() const { return *this; }
    template <class F>
    void convert_values(const F &from, rvec to) const {
        std::visit([&](const auto &c) { c.convert_values(from, to); }, converter);
    }
    void convert_values(crvec from, rvec to) const = delete;

  private:
    static auto wrap_converter(Request request) {
        return [request]<class From>(const From &from) -> ConverterVariant<To> {
            return SparsityConverter<From, To>{from, request};
        };
    }
};

} // namespace alpaqa::sparsity