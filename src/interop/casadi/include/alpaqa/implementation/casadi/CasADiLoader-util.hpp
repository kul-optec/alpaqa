#pragma once

#include <alpaqa/casadi/CasADiFunctionWrapper.hpp>
#include <alpaqa/util/demangled-typename.hpp>
#include <casadi/core/casadi_types.hpp>
#include <array>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace alpaqa::casadi_loader {

template <class Loader, class F>
auto wrap_load(Loader &&loader, const char *name, F f) {
    try {
        return f();
    } catch (const invalid_argument_dimensions &e) {
        throw std::invalid_argument(
            "Unable to load function '" + loader.format_name(name) +
            "': " + demangled_typename(typeid(e)) + ": " + e.what());
    }
}

template <class T, class Loader, class... Args>
auto wrapped_load(Loader &&loader, const char *name, Args &&...args) {
    return wrap_load(loader, name, [&] {
        return T(loader(name), std::forward<Args>(args)...);
    });
}

template <class T, class Loader, class... Args>
std::optional<T> try_load(Loader &&loader, const char *name, Args &&...args) {
    try {
        return std::make_optional(wrapped_load<T>(
            std::forward<Loader>(loader), name, std::forward<Args>(args)...));
    } catch (casadi::CasadiException &) {
        return std::nullopt;
    } catch (std::out_of_range &) {
        // TODO: can be made more robust against false positives
        return std::nullopt;
    }
}

using dim = std::pair<casadi_int, casadi_int>;
inline constexpr auto dims(auto... a) {
    if constexpr ((... && std::is_constructible_v<dim, decltype(a)>))
        return std::array{a...};
    else
        return std::array{dim{a, 1}...};
}

} // namespace alpaqa::casadi_loader
