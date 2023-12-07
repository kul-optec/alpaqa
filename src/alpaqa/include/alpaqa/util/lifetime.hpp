#pragma once

#include <memory>

#if __cpp_lib_start_lifetime_as >= 202207L

namespace alpaqa::util {
using std::start_lifetime_as;
using std::start_lifetime_as_array;
} // namespace alpaqa::util

#else

#include <cstring>
#include <new>
#include <type_traits>

namespace alpaqa::util {
template <class T>
    requires std::is_trivially_copyable_v<T>
T *start_lifetime_as_array(void *p, size_t n) noexcept {
#if __cpp_lib_is_implicit_lifetime >= 202302L
    static_assert(std::is_implicit_lifetime_v<T>);
#endif
    return std::launder(static_cast<T *>(std::memmove(p, p, n * sizeof(T))));
}
template <class T>
    requires std::is_trivially_copyable_v<T>
const T *start_lifetime_as_array(const void *p, size_t n) noexcept {
#if __cpp_lib_is_implicit_lifetime >= 202302L
    static_assert(std::is_implicit_lifetime_v<T>);
#endif
    static_cast<void>(n); // TODO
    // best we can do without compiler support
    return std::launder(static_cast<const T *>(p));
}
template <class T>
    requires std::is_trivially_copyable_v<T>
T *start_lifetime_as(void *p) noexcept {
    return start_lifetime_as_array<T>(p, 1);
}
template <class T>
    requires std::is_trivially_copyable_v<T>
const T *start_lifetime_as(const void *p) noexcept {
    return start_lifetime_as_array<T>(p, 1);
}
} // namespace alpaqa::util

#endif
