#pragma once

#include <algorithm>
#include <array>

namespace alpaqa::cutest {

/// Compile-time string for CUTEst function names.
template <size_t N>
struct Name {
    constexpr Name(const char (&str)[N]) { // NOLINT(*-c-arrays)
        std::copy_n(str, N, value.data());
    }
    std::array<char, N> value;
};

/// Reference to CUTEst function.
template <Name Nm, class Sgn>
struct Function {
    using signature_t          = Sgn;
    static constexpr Name name = Nm;
    [[nodiscard]] static signature_t *load(void *handle);
};

using integer                = int;
using doublereal             = double;
using logical                = int;
constexpr logical True       = 1;
constexpr logical False      = 0;
constexpr doublereal inf     = 1e20;
constexpr size_t fstring_len = 10;

} // namespace alpaqa::cutest
