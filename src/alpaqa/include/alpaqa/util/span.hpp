#pragma once

#include <Eigen/Core>
#include <span>
#include <type_traits>

namespace alpaqa {

namespace detail {
template <Eigen::Index R>
constexpr auto to_std_extent =
    R == Eigen::Dynamic ? std::dynamic_extent : static_cast<size_t>(R);
template <size_t E>
constexpr auto to_eigen_extent =
    E == std::dynamic_extent ? Eigen::Dynamic : static_cast<Eigen::Index>(E);
} // namespace detail

/// Convert an Eigen vector view to a `std::span`.
template <class Derived>
    requires(Derived::ColsAtCompileTime == 1)
auto as_span(Eigen::DenseBase<Derived> &v) {
    constexpr auto E = detail::to_std_extent<Derived::RowsAtCompileTime>;
    using T          = std::remove_pointer_t<decltype(v.derived().data())>;
    return std::span<T, E>{v.derived().data(), static_cast<size_t>(v.size())};
}

/// Convert an Eigen vector view to a `std::span`.
template <class Derived>
    requires(Derived::ColsAtCompileTime == 1)
auto as_span(const Eigen::DenseBase<Derived> &v) {
    constexpr auto E = detail::to_std_extent<Derived::RowsAtCompileTime>;
    using T          = std::remove_pointer_t<decltype(v.derived().data())>;
    return std::span<T, E>{v.derived().data(), static_cast<size_t>(v.size())};
}

/// Convert a `std::span` to an Eigen::Vector view.
template <class T, size_t E>
auto as_vec(std::span<T, E> s) {
    constexpr auto R = detail::to_eigen_extent<E>;
    using S          = std::remove_const_t<T>;
    using V          = Eigen::Vector<S, R>;
    using Map = Eigen::Map<std::conditional_t<std::is_const_v<T>, const V, V>>;
    if constexpr (R == Eigen::Dynamic)
        return Map{s.data(), static_cast<Eigen::Index>(s.size())};
    else
        return Map{s.data()};
}

} // namespace alpaqa
