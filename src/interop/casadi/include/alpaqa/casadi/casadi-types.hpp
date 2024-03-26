#pragma once

#include <algorithm>
#include <array>
#include <string>

#include "casadi-namespace.hpp"

namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE {

#ifndef casadi_int
using casadi_int = long long int;
#endif
#ifndef casadi_real
using casadi_real = double;
#endif

namespace casadi {

/// Compile-time string for CasADi function names.
template <size_t N>
struct Name {
    constexpr Name(const char (&str)[N]) { // NOLINT(*-c-arrays)
        std::copy_n(str, N, value.data());
    }
    std::array<char, N> value;
};

/// Reference to CasADi function.
template <Name Nm, class Sgn>
struct ExternalFunction {
    using signature_t          = Sgn;
    static constexpr Name name = Nm;
    [[nodiscard]] static signature_t *load(void *handle, std::string fname);
};

} // namespace casadi

} // namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE
