#pragma once

#include "custom-config.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <type_traits>

namespace custom {

// CustomVectorView ------------------------------------------------------------

template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::dot(
    CustomVectorView<const T, ElementWise> o) const -> value_type {
    assert(o.size() == this->size());
    return std::inner_product(begin(), end(), o.begin(), value_type{0});
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator+(
    CustomVectorView<const T, ElementWise> o) const -> vec {
    assert(o.size() == this->size());
    vec v(o.size());
    std::ranges::transform(*this, o, v.begin(), std::plus{});
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator+=(
    CustomVectorView<const T, ElementWise> o) -> CustomVectorView &
    requires(!std::is_const_v<T>)
{
    assert(o.size() == this->size());
    std::ranges::transform(*this, o, begin(), std::plus{});
    return *this;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator-(
    CustomVectorView<const T, ElementWise> o) const -> vec {
    assert(o.size() == this->size());
    vec v(o.size());
    std::ranges::transform(*this, o, v.begin(), std::minus{});
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator-=(
    CustomVectorView<const T, ElementWise> o) -> CustomVectorView &
    requires(!std::is_const_v<T>)
{
    assert(o.size() == this->size());
    std::ranges::transform(*this, o, begin(), std::minus{});
    return *this;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator-() const -> vec {
    vec v(size());
    std::ranges::transform(*this, v.begin(), std::negate{});
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::cwiseProduct(
    CustomVectorView<const T, ElementWise> o) const -> vec {
    assert(o.size() == this->size());
    vec v(o.size());
    std::ranges::transform(*this, o, v.begin(), std::multiplies{});
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator*(
    CustomVectorView<const T, ElementWise> o) const -> vec
    requires ElementWise
{
    return cwiseProduct(o);
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator*=(
    CustomVectorView<const T, ElementWise> o)
    -> CustomVectorView &requires(ElementWise && !std::is_const_v<T>) {
    assert(o.size() == this->size());
    std::ranges::transform(*this, o, begin(), std::multiplies{});
    return *this;
}

template <class T, bool ElementWise>
template <class U>
auto CustomVectorView<T, ElementWise>::operator*=(const U &o)
    -> CustomVectorView &
    requires(!std::is_const_v<T>)
{
    auto mul = [&o](const auto &a) { return a * o; };
    std::ranges::transform(*this, begin(), mul);
    return *this;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::cwiseQuotient(
    CustomVectorView<const T, ElementWise> o) const -> vec {
    assert(o.size() == this->size());
    vec v(o.size());
    std::ranges::transform(*this, o, v.begin(), std::divides{});
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator/=(
    CustomVectorView<const T, ElementWise> o)
    -> CustomVectorView &requires(ElementWise && !std::is_const_v<T>) {
    assert(o.size() == this->size());
    std::ranges::transform(*this, o, begin(), std::divides{});
    return *this;
}

template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator/(
    CustomVectorView<const T, ElementWise> o) const -> vec
    requires ElementWise
{
    return cwiseQuotient(o);
}

template <class T, bool ElementWise>
template <class U>
auto CustomVectorView<T, ElementWise>::operator/=(const U &o)
    -> CustomVectorView &
    requires(!std::is_const_v<T>)
{
    auto div = [&o](const auto &a) { return a / o; };
    std::ranges::transform(*this, begin(), div);
    return *this;
}

template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator==(
    CustomVectorView<const T, ElementWise> o) const -> bool
    requires(!ElementWise)
{
    assert(o.size() == this->size());
    return std::ranges::equal(*this, o);
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator!=(
    CustomVectorView<const T, ElementWise> o) const -> bool
    requires(!ElementWise)
{
    return !(*this == o);
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator==(
    CustomVectorView<const T, ElementWise> o) const -> CustomVector<bool>
    requires ElementWise
{
    assert(o.size() == this->size());
    CustomVector<bool> v(o.size());
    std::ranges::transform(*this, o, v.begin(), std::equal_to{});
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator!=(
    CustomVectorView<const T, ElementWise> o) const -> CustomVector<bool>
    requires ElementWise
{
    assert(o.size() == this->size());
    CustomVector<bool> v(o.size());
    std::ranges::transform(*this, o, v.begin(), std::not_equal_to{});
    return v;
}
template <class T, bool ElementWise>
template <class U>
auto CustomVectorView<T, ElementWise>::operator==(const U &b) const
    -> CustomVector<bool>
    requires(ElementWise && std::equality_comparable_with<T, U>)
{
    CustomVector<bool> v(size());
    auto eq = [&b](const auto &a) { return a == b; };
    std::ranges::transform(*this, v.begin(), eq);
    return v;
}
template <class T, bool ElementWise>
template <class U>
auto CustomVectorView<T, ElementWise>::operator!=(const U &b) const
    -> CustomVector<bool>
    requires(ElementWise && std::equality_comparable_with<T, U>)
{
    CustomVector<bool> v(size());
    auto eq = [&b](const auto &a) { return a != b; };
    std::ranges::transform(*this, v.begin(), eq);
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::allFinite() const -> bool {
    return std::ranges::all_of(
        *this, [](const auto &v) -> bool { return std::isfinite(v); });
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::squaredNorm() const -> value_type {
    return dot(*this);
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::norm() const -> value_type {
    using std::sqrt;
    return sqrt(squaredNorm());
}

template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::cwiseMax(
    CustomVectorView<const T, ElementWise> o) const -> vec {
    assert(o.size() == this->size());
    vec v(o.size());
    auto max = [](const T &a, const T &b) {
        if constexpr (std::is_floating_point_v<T>)
            return std::fmax(a, b);
        return std::max(a, b);
    };
    std::ranges::transform(*this, o, v.begin(), max);
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::cwiseMin(
    CustomVectorView<const T, ElementWise> o) const -> vec {
    assert(o.size() == this->size());
    vec v(o.size());
    auto min = [](const T &a, const T &b) {
        if constexpr (std::is_floating_point_v<T>)
            return std::fmin(a, b);
        return std::min(a, b);
    };
    std::ranges::transform(*this, o, v.begin(), min);
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::cwiseMax(const T &o) const -> vec {
    vec v(size());
    auto max = [&o](const T &a) {
        if constexpr (std::is_floating_point_v<T>)
            return std::fmax(a, o);
        return std::max(a, o);
    };
    std::ranges::transform(*this, v.begin(), max);
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::cwiseMin(const T &o) const -> vec {
    vec v(size());
    auto min = [&o](const T &a) {
        if constexpr (std::is_floating_point_v<T>)
            return std::fmin(a, o);
        return std::min(a, o);
    };
    std::ranges::transform(*this, v.begin(), min);
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::cwiseAbs() const -> vec {
    vec v(size());
    auto abs_ = [](const T &a) {
        using std::abs;
        return abs(a);
    };
    std::ranges::transform(*this, v.begin(), abs_);
    return v;
}

template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator<(const T &o) const
    -> CustomVector<bool> {
    CustomVector<bool> v(size());
    std::ranges::transform(*this, v.begin(),
                           [&o](const T &a) { return std::less{}(a, o); });
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator<(
    CustomVectorView<const T, ElementWise> o) const -> CustomVector<bool> {
    assert(o.size() == this->size());
    CustomVector<bool> v(size());
    std::ranges::transform(*this, o, v.begin(), std::less{});
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator<=(
    CustomVectorView<const T, ElementWise> o) const -> CustomVector<bool> {
    assert(o.size() == this->size());
    CustomVector<bool> v(size());
    std::ranges::transform(*this, o, v.begin(), std::less_equal{});
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator>(const T &o) const
    -> CustomVector<bool> {
    CustomVector<bool> v(size());
    std::ranges::transform(*this, v.begin(),
                           [&o](const T &a) { return std::greater{}(a, o); });
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator>(
    CustomVectorView<const T, ElementWise> o) const -> CustomVector<bool> {
    assert(o.size() == this->size());
    CustomVector<bool> v(size());
    std::ranges::transform(*this, o, v.begin(), std::greater{});
    return v;
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::operator>=(
    CustomVectorView<const T, ElementWise> o) const -> CustomVector<bool> {
    assert(o.size() == this->size());
    CustomVector<bool> v(size());
    std::ranges::transform(*this, o, v.begin(), std::greater_equal{});
    return v;
}

template <class T, bool ElementWise>
template <class U>
auto CustomVectorView<T, ElementWise>::select(CustomVectorView<const U> a,
                                              CustomVectorView<const U> b) const
    -> CustomVector<U> {
    assert(a.size() == this->size());
    assert(b.size() == this->size());
    CustomVector<U> v(size());
    for (index_type i = 0; i < size(); ++i)
        v(i) = (*this)(i) ? a(i) : b(i);
    return v;
}
template <class T, bool ElementWise>
template <class U>
auto CustomVectorView<T, ElementWise>::select(
    const CustomVector<U> &a, CustomVectorView<U> b) const -> CustomVector<U> {
    return select(CustomVectorView<const U>{a}, CustomVectorView<const U>{b});
}
template <class T, bool ElementWise>
template <class U>
auto CustomVectorView<T, ElementWise>::select(const CustomVector<U> &a,
                                              CustomVectorView<const U> b) const
    -> CustomVector<U> {
    return select(CustomVectorView<const U>{a}, b);
}
template <class T, bool ElementWise>
template <class U>
auto CustomVectorView<T, ElementWise>::select(
    CustomVectorView<const U> a, const U &b) const -> CustomVector<U> {
    assert(a.size() == this->size());
    CustomVector<U> v(size());
    for (index_type i = 0; i < size(); ++i)
        v(i) = (*this)(i) ? a(i) : b;
    return v;
}

template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::topRows(index_type n) const
    -> CustomVectorView<T> {
    return {begin(), n};
}
template <class T, bool ElementWise>
auto CustomVectorView<T, ElementWise>::bottomRows(index_type n) const
    -> CustomVectorView<T> {
    return {end() - n, n};
}

// CustomVector ----------------------------------------------------------------

template <class T>
CustomVector<T> &CustomVector<T>::operator=(CustomVectorView<const T> o) {
    resize(o.size());
    std::ranges::copy(o, begin());
    return *this;
}
template <class T>
CustomVector<T> &CustomVector<T>::operator=(CustomVectorView<T> o) {
    return *this = CustomVectorView<const T>{o};
}
template <class T>
CustomVector<T> &CustomVector<T>::operator=(const CustomVector<T> &o) {
    resize(o.size());
    std::ranges::copy(o, begin());
    return *this;
}

template <class T>
CustomVector<T> CustomVector<T>::Constant(index_type size, value_type value) {
    CustomVector<T> v(size);
    v.setConstant(value);
    return v;
}
template <class T>
CustomVector<T> CustomVector<T>::Zero(index_type size) {
    return Constant(size, T{0});
}
template <class T>
CustomVector<T> CustomVector<T>::Ones(index_type size) {
    return Constant(size, T{1});
}

template <class T>
void CustomVector<T>::resize(index_type size) {
    if (size != this->size())
        v = std::make_unique<T[]>(cast_index(size));
}

template <class T>
void CustomVector<T>::swap(CustomVector<T> &o) noexcept {
    using std::swap;
    static_assert(noexcept(swap(v, o.v)));
    swap(v, o.v);
}

template <class T>
auto CustomVector<T>::dot(CustomVectorView<const T> o) -> value_type {
    return CustomVectorView<const T>{*this}.dot(o);
}
template <class T>
auto CustomVector<T>::operator+(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this} + o;
}
template <class T>
auto CustomVector<T>::operator+=(CustomVectorView<const T> o) const -> vec & {
    return CustomVectorView<T>{*this} += o, *this;
}
template <class T>
auto CustomVector<T>::operator-(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this} - o;
}
template <class T>
auto CustomVector<T>::operator-=(CustomVectorView<const T> o) const -> vec & {
    return CustomVectorView<T>{*this} -= o, *this;
}
template <class T>
auto CustomVector<T>::operator-() const -> vec {
    return -CustomVectorView<const T>{*this};
}
template <class T>
auto CustomVector<T>::cwiseProduct(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseProduct(o);
}
template <class T>
auto CustomVector<T>::cwiseQuotient(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseQuotient(o);
}
template <class T>
auto CustomVector<T>::operator==(CustomVectorView<const T> o) const -> bool {
    return CustomVectorView<const T>{*this} == o;
}
template <class T>
auto CustomVector<T>::operator!=(CustomVectorView<const T> o) const -> bool {
    return CustomVectorView<const T>{*this} != o;
}
template <class T>
auto CustomVector<T>::allFinite() const -> bool {
    return CustomVectorView<const T>{*this}.allFinite();
}
template <class T>
auto CustomVector<T>::squaredNorm() const -> value_type {
    return CustomVectorView<const T>{*this}.squaredNorm();
}
template <class T>
auto CustomVector<T>::norm() const -> value_type {
    return CustomVectorView<const T>{*this}.norm();
}
template <class T>
auto CustomVector<T>::cwiseMax(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseMax(o);
}
template <class T>
auto CustomVector<T>::cwiseMin(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseMin(o);
}
template <class T>
auto CustomVector<T>::cwiseMax(const T &o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseMax(o);
}
template <class T>
auto CustomVector<T>::cwiseMin(const T &o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseMin(o);
}
template <class T>
auto CustomVector<T>::cwiseAbs() const -> vec {
    return CustomVectorView<const T>{*this}.cwiseAbs();
}

template <class T>
auto CustomVector<T>::array() -> CustomVectorView<T, true> {
    return {*this};
}
template <class T>
auto CustomVector<T>::array() const -> CustomVectorView<const T, true> {
    return {*this};
}

template <class T>
template <class U>
CustomVector<U> CustomVector<T>::select(CustomVectorView<const U> a,
                                        CustomVectorView<const U> b) const {
    return CustomVectorView<const T>{*this}.select(a, b);
}
template <class T>
template <class U>
CustomVector<U> CustomVector<T>::select(const CustomVector<U> &a,
                                        CustomVectorView<U> b) const {
    return CustomVectorView<const T>{*this}.select(a, b);
}
template <class T>
template <class U>
CustomVector<U> CustomVector<T>::select(const CustomVector<U> &a,
                                        CustomVectorView<const U> b) const {
    return CustomVectorView<const T>{*this}.select(a, b);
}
template <class T>
template <class U>
CustomVector<U> CustomVector<T>::select(const CustomVector<U> &a,
                                        const CustomVector<U> &b) const {
    return CustomVectorView<const T>{*this}.select(
        CustomVectorView<const U>{a}, CustomVectorView<const U>{b});
}
template <class T>
template <class U>
CustomVector<U> CustomVector<T>::select(const CustomVector<U> &a,
                                        const U &b) const {
    return CustomVectorView<const T>{*this}.select(CustomVectorView<const U>{a},
                                                   b);
}

template <class T>
auto CustomVector<T>::topRows(index_type n) const -> CustomVectorView<const T> {
    return CustomVectorView<const T>{*this}.topRows(n);
}
template <class T>
auto CustomVector<T>::bottomRows(index_type n) const
    -> CustomVectorView<const T> {
    return CustomVectorView<const T>{*this}.bottomRows(n);
}
template <class T>
auto CustomVector<T>::topRows(index_type n) -> CustomVectorView<T> {
    return CustomVectorView<T>{*this}.topRows(n);
}
template <class T>
auto CustomVector<T>::bottomRows(index_type n) -> CustomVectorView<T> {
    return CustomVectorView<T>{*this}.bottomRows(n);
}

// free function operators

template <class A, class B, bool E>
auto operator*(const A &a, CustomVectorView<B, E> o) {
    CustomVector<decltype(a * std::declval<B>())> v(o.size());
    std::ranges::transform(o, v.begin(), [&a](const auto &b) { return a * b; });
    return v;
}
template <class A, class B, bool E>
auto operator*(CustomVectorView<A, E> a, const B &b) {
    CustomVector<decltype(std::declval<A>() * b)> v(a.size());
    std::ranges::transform(a, v.begin(), [&b](const auto &a) { return a * b; });
    return v;
}
template <class A, class B>
auto operator*(const A &a, CustomVector<B> o) {
    return a * CustomVectorView<const B>{o};
}
template <class A, class B>
auto operator*(CustomVector<A> a, const B &b) {
    return CustomVectorView<const B>{a} * b;
}

template <class T>
T norm_inf(CustomVectorView<T> v) {
    auto abs_ = [](const T &a) {
        using std::abs;
        return abs(a);
    };
    auto max_ = [](const T &a, const T &b) {
        using std::max;
        return max(a, b);
    };
    return std::transform_reduce(v.begin(), v.end(), T{0}, max_, abs_);
}
template <class T>
T norm_inf(const CustomVector<T> &v) {
    return norm_inf(CustomVectorView<const T>{v});
}

template <class T>
T norm_1(CustomVectorView<T> v) {
    auto abs_ = [](const T &a) {
        using std::abs;
        return abs(a);
    };
    return std::transform_reduce(v.begin(), v.end(), T{0}, std::plus{}, abs_);
}
template <class T>
T norm_1(const CustomVector<T> &v) {
    return norm_1(CustomVectorView<const T>{v});
}

} // namespace custom
