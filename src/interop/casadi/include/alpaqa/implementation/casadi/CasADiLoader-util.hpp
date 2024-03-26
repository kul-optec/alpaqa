#pragma once

#include <alpaqa/casadi/CasADiFunctionWrapper.hpp>
#include <alpaqa/casadi/casadi-namespace.hpp>
#include <alpaqa/util/demangled-typename.hpp>
#include <alpaqa/util/dl.hpp>

#include <array>
#include <optional>
#include <stdexcept>
#include <utility>

#if ALPAQA_WITH_EXTERNAL_CASADI
#include <casadi/core/casadi_types.hpp>
#else
#include <alpaqa/casadi/casadi-types.hpp>
#endif

namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi_loader {

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
#if ALPAQA_WITH_EXTERNAL_CASADI
    } catch (casadi::CasadiException &) {
        return std::nullopt;
#else
    } catch (util::dynamic_load_error &e) {
        return std::nullopt;
#endif
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

} // namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi_loader
