#pragma once

#include <type_traits>

namespace alpaqa::util {
namespace detail {

/// Check if @p A is equal to any of @p Bs.
template <class A, class... Bs>
constexpr bool any_is_same() {
    return (std::is_same_v<A, Bs> || ...);
}

/// Unused unique type tag for template specializations that were rejected
/// because some types were not distinct.
template <class...>
struct dummy;

} // namespace detail

/// If @p NewAlias is not the same type as any of @p PossibleAliases, the result
/// is @p NewAlias. If @p NewAlias is not distinct from @p PossibleAliases, the
/// result is a dummy type, uniquely determined by @p NewAlias and
/// @p PossibleAliases.
template <class NewAlias, class... PossibleAliases>
using possible_alias_t =
    std::conditional_t<detail::any_is_same<NewAlias, PossibleAliases...>(),
                       detail::dummy<NewAlias, PossibleAliases...>, NewAlias>;

} // namespace alpaqa::util
