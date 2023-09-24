#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/sparsity.hpp>
#include <alpaqa/util/sparse-ops.hpp>

#include <algorithm>
#include <cassert>
#include <numeric>
#include <optional>
#include <stdexcept>

namespace alpaqa::sparsity {

template <class From, class To>
struct SparsityConverter;

template <class To>
struct SparsityConversionRequest;

template <Config Conf>
struct SparsityConversionRequest<Dense<Conf>> {};

template <Config Conf>
struct SparsityConverter<Dense<Conf>, Dense<Conf>> {
    USING_ALPAQA_CONFIG(Conf);
    using from_sparsity_t = Dense<Conf>;
    using to_sparsity_t   = Dense<Conf>;
    SparsityConverter(from_sparsity_t from) : sparsity(std::move(from)) {}
    to_sparsity_t sparsity;
    operator const to_sparsity_t &() const & { return sparsity; }
    operator to_sparsity_t &() & { return sparsity; }
    operator to_sparsity_t &&() && { return std::move(sparsity); }
    void convert_values(crvec from, rvec to) const {
        if (to.data() != from.data())
            to = from;
    }
};

template <Config Conf>
struct SparsityConversionRequest<SparseCOO<Conf>> {
    // TODO: probably not needed.
    // std::optional<typename SparseCOO<Conf>::Order> order = std::nullopt;
};

template <Config Conf, class StorageIndex>
struct SparsityConverter<Dense<Conf>, SparseCOO<Conf, StorageIndex>> {
    USING_ALPAQA_CONFIG(Conf);
    using from_sparsity_t = Dense<Conf>;
    using to_sparsity_t   = SparseCOO<Conf, StorageIndex>;
    using storage_index_t = typename to_sparsity_t::storage_index_t;
    using index_vector_t  = typename to_sparsity_t::index_vector_t;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity(from_sparsity_t from) {
        switch (from.symmetry) {
            case Symmetry::Unsymmetric: {
                length_t nnz = from.rows * from.cols;
                row_indices.resize(nnz);
                col_indices.resize(nnz);
                index_t l = 0;
                for (index_t c = 0; c < from.cols; ++c) {
                    for (index_t r = 0; r < from.rows; ++r) {
                        row_indices[l] = static_cast<storage_index_t>(r);
                        col_indices[l] = static_cast<storage_index_t>(c);
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
                        row_indices[l] = static_cast<storage_index_t>(r);
                        col_indices[l] = static_cast<storage_index_t>(c);
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
            .first_index = 0,
        };
    }
    SparsityConverter(from_sparsity_t from, Request = {}) : sparsity(convert_sparsity(from)) {}
    index_vector_t row_indices, col_indices;
    to_sparsity_t sparsity;
    operator const to_sparsity_t &() const & { return sparsity; }
    operator to_sparsity_t &() & { return sparsity; }
    operator to_sparsity_t &&() && = delete;
    void convert_values(crvec from, rvec to) const {
        if (sparsity.symmetry == Symmetry::Unsymmetric) {
            if (to.data() != from.data())
                to = from;
        } else if (sparsity.symmetry == Symmetry::Upper) {
            auto &&F = from.reshaped(sparsity.rows, sparsity.cols);
            auto t   = to.begin();
            for (index_t c = 0; c < sparsity.cols; ++c)
                std::ranges::copy_backward(F.col(c).topRows(c + 1), t += c + 1);
        } else {
            assert(false);
        }
    }
};

template <Config Conf, class StorageIndex>
struct SparsityConverter<SparseCSC<Conf>, SparseCOO<Conf, StorageIndex>> {
    USING_ALPAQA_CONFIG(Conf);
    using from_sparsity_t = SparseCSC<Conf>;
    using to_sparsity_t   = SparseCOO<Conf, StorageIndex>;
    using storage_index_t = typename to_sparsity_t::storage_index_t;
    using index_vector_t  = typename to_sparsity_t::index_vector_t;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity(from_sparsity_t from) {
        row_indices.resize(from.nnz());
        col_indices.resize(from.nnz());
        index_t l = 0;
        for (index_t c = 0; c < from.cols; ++c) {
            auto inner_start = from.outer_ptr(c);
            auto inner_end   = from.outer_ptr(c + 1);
            for (auto i = inner_start; i < inner_end; ++i) {
                auto r         = from.inner_idx(i);
                row_indices[l] = static_cast<storage_index_t>(r);
                col_indices[l] = static_cast<storage_index_t>(c);
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
            .first_index = 0,
        };
    }
    SparsityConverter(from_sparsity_t from, Request = {}) : sparsity(convert_sparsity(from)) {}
    index_vector_t row_indices, col_indices;
    to_sparsity_t sparsity;
    operator const to_sparsity_t &() const & { return sparsity; }
    operator to_sparsity_t &() & { return sparsity; }
    operator to_sparsity_t &&() && = delete;
    void convert_values(crvec from, rvec to) const {
        if (to.data() != from.data())
            to = from;
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
    to_sparsity_t convert_sparsity(from_sparsity_t from) {
        row_indices.resize(from.nnz());
        col_indices.resize(from.nnz());
        auto cvt_idx = [](auto i) { return static_cast<storage_index_t>(i); };
        std::ranges::transform(from.row_indices, row_indices.begin(), cvt_idx);
        std::ranges::transform(from.col_indices, col_indices.begin(), cvt_idx);
        return {
            .rows        = from.rows,
            .cols        = from.cols,
            .symmetry    = from.symmetry,
            .row_indices = row_indices,
            .col_indices = col_indices,
            .order       = static_cast<typename to_sparsity_t::Order>(from.order),
            .first_index = cvt_idx(from.first_index),
        };
    }
    SparsityConverter(from_sparsity_t from, Request = {}) : sparsity(convert_sparsity(from)) {}
    index_vector_t row_indices, col_indices;
    to_sparsity_t sparsity;
    operator const to_sparsity_t &() const & { return sparsity; }
    operator to_sparsity_t &() & { return sparsity; }
    operator to_sparsity_t &&() && = delete;
    void convert_values(crvec from, rvec to) const {
        if (to.data() != from.data())
            to = from;
    }
};

template <Config Conf>
struct SparsityConversionRequest<SparseCSC<Conf>> {
    std::optional<typename SparseCSC<Conf>::Order> order = std::nullopt;
};

template <Config Conf, class StorageIndex>
struct SparsityConverter<SparseCOO<Conf, StorageIndex>, SparseCSC<Conf>> {
    USING_ALPAQA_CONFIG(Conf);
    using to_sparsity_t   = SparseCSC<Conf>;
    using from_sparsity_t = SparseCOO<Conf, StorageIndex>;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity([[maybe_unused]] from_sparsity_t from,
                                   [[maybe_unused]] Request request) {
#if ALPAQA_HAVE_COO_CSC_CONVERSIONS
        // Convert the indices
        // TODO: using indexvec here could be suboptimal if the original storage
        //       index was int
        indexvec row_indices(from.nnz()), col_indices(from.nnz());
        auto cvt_idx = [](auto i) { return static_cast<index_t>(i); };
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
        util::convert_triplets_to_ccs<config_t>(row_indices, col_indices, inner_idx, outer_ptr,
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
        : sparsity(convert_sparsity(from, request)) {}
    indexvec inner_idx, outer_ptr, permutation;
    to_sparsity_t sparsity;
    operator const to_sparsity_t &() const & { return sparsity; }
    operator to_sparsity_t &() & { return sparsity; }
    operator to_sparsity_t &&() && = delete;
    void convert_values(crvec from, rvec to) const {
        if (permutation.size() > 0) {
            assert(to.data() != from.data());
            to = from(permutation);
        } else {
            if (to.data() != from.data())
                to = from;
        }
    }
};

template <Config Conf>
struct SparsityConverter<SparseCSC<Conf>, SparseCSC<Conf>> {
    USING_ALPAQA_CONFIG(Conf);
    using to_sparsity_t   = SparseCSC<Conf>;
    using from_sparsity_t = SparseCSC<Conf>;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity([[maybe_unused]] from_sparsity_t from, Request request) {
        // Sort the indices
        typename to_sparsity_t::Order order = from.order;
        if (request.order && *request.order == to_sparsity_t::SortedRows) {
            order = to_sparsity_t::SortedRows;
            switch (from.order) {
                case from_sparsity_t::Unsorted:
#if ALPAQA_HAVE_COO_CSC_CONVERSIONS
                    inner_idx = from.inner_idx;
                    outer_ptr = from.outer_ptr;
                    permutation.resize(from.nnz());
                    std::iota(permutation.begin(), permutation.end(), index_t{0});
                    util::sort_rows_csc(outer_ptr, inner_idx, permutation);
#else
                    throw std::runtime_error(
                        "This build of alpaqa does not support sorting matrices in CSC format. "
                        "Please recompile with a C++23-compliant compiler.");
#endif
                    break;
                case from_sparsity_t::SortedRows:
                    // No sorting necessary
                    break;
                default: throw std::invalid_argument("Invalid order");
            }
        }
        assert(!request.order || *request.order == order);
        if (std::ranges::is_sorted(permutation))
            permutation = indexvec{};
        return {
            .rows      = from.rows,
            .cols      = from.cols,
            .symmetry  = from.symmetry,
            .inner_idx = inner_idx.size() > 0 ? crindexvec{inner_idx} : from.inner_idx,
            .outer_ptr = outer_ptr.size() > 0 ? crindexvec{outer_ptr} : from.outer_ptr,
            .order     = order,
        };
    }
    SparsityConverter(from_sparsity_t from, Request request = {})
        : sparsity(convert_sparsity(from, request)) {}
    indexvec inner_idx, outer_ptr, permutation;
    to_sparsity_t sparsity;
    operator const to_sparsity_t &() const & { return sparsity; }
    operator to_sparsity_t &() & { return sparsity; }
    operator to_sparsity_t &&() && = delete;
    void convert_values(crvec from, rvec to) const {
        if (permutation.size() > 0) {
            assert(to.data() != from.data());
            to = from(permutation);
        } else {
            if (to.data() != from.data())
                to = from;
        }
    }
};

template <Config Conf>
struct SparsityConverter<Dense<Conf>, SparseCSC<Conf>> {
    USING_ALPAQA_CONFIG(Conf);
    using to_sparsity_t   = SparseCSC<Conf>;
    using from_sparsity_t = Dense<Conf>;
    using Request         = SparsityConversionRequest<to_sparsity_t>;
    to_sparsity_t convert_sparsity([[maybe_unused]] from_sparsity_t from, Request) {
        switch (from.symmetry) {
            case Symmetry::Unsymmetric: {
                inner_idx.resize(from.rows * from.cols);
                outer_ptr.resize(from.cols + 1);
                index_t l = 0;
                for (index_t c = 0; c < from.cols; ++c) {
                    outer_ptr[c] = l;
                    for (index_t r = 0; r < from.rows; ++r) {
                        inner_idx[l] = r;
                        ++l;
                    }
                }
                outer_ptr[from.cols] = l;
            } break;
            case Symmetry::Upper: {
                if (from.rows != from.cols)
                    throw std::invalid_argument("Nonsquare matrix cannot be symmetric");
                inner_idx.resize(from.rows * (from.rows + 1) / 2);
                outer_ptr.resize(from.cols + 1);
                index_t l = 0;
                for (index_t c = 0; c < from.cols; ++c) {
                    outer_ptr[c] = l;
                    for (index_t r = 0; r <= c; ++r) {
                        inner_idx[l] = r;
                        ++l;
                    }
                }
                outer_ptr[from.cols] = l;
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
        : sparsity(convert_sparsity(from, request)) {}
    indexvec inner_idx, outer_ptr;
    to_sparsity_t sparsity;
    operator const to_sparsity_t &() const & { return sparsity; }
    operator to_sparsity_t &() & { return sparsity; }
    operator to_sparsity_t &&() && = delete;
    void convert_values(crvec from, rvec to) const {
        if (sparsity.symmetry == Symmetry::Unsymmetric) {
            if (to.data() != from.data())
                to = from;
        } else if (sparsity.symmetry == Symmetry::Upper) {
            auto &&F = from.reshaped(sparsity.rows, sparsity.cols);
            auto t   = to.begin();
            for (index_t c = 0; c < sparsity.cols; ++c)
                std::ranges::copy_backward(F.col(c).topRows(c + 1), t += c + 1);
        } else {
            assert(false);
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

template <class To>
struct ConvertedSparsity {
    USING_ALPAQA_CONFIG_TEMPLATE(To::config_t);
    using to_sparsity_t = To;
    using Request       = SparsityConversionRequest<to_sparsity_t>;
    template <class... Args>
    static auto wrap_converter(Args &&...args) {
        return [&args...]<class From>(const From &from) -> ConverterVariant<To> {
            return SparsityConverter<From, To>{from, std::forward<Args>(args)...};
        };
    }
    ConvertedSparsity(Sparsity<config_t> from, Request request = {})
        : converter{std::visit(wrap_converter(std::move(request)), from)} {}
    ConverterVariant<To> converter;
    operator const To &() const {
        return std::visit([](const auto &c) -> const To & { return c; }, converter);
    }
    const To &get_sparsity() const { return *this; }
    void convert_values(crvec from, rvec to) const {
        std::visit([&](const auto &c) { c.convert_values(std::move(from), to); }, converter);
    }
};

} // namespace alpaqa::sparsity