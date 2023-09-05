#pragma once

#include <type_traits>
#include <utility>

/// @file
/// @see https://wg21.link/P1895R0
/// @see https://wg21.link/P2547R0

namespace alpaqa {

namespace tag_invoke_fn_ns {

/// Poison pill to hide overloads of foo() that might be found in parent
/// namespace.
/// We want to limit to only finding overloads by ADL.
#if defined(_MSC_VER) || (defined(__GNUC__) && __GNUC__ < 12 && !defined(__clang__))
void alpaqa_tag_invoke() /* = delete */;
#else
void alpaqa_tag_invoke() = delete;
#endif

struct tag_invoke_fn {
    template <typename Tag, typename... Args>
        requires requires(Tag tag, Args &&...args) {
            alpaqa_tag_invoke(std::move(tag), std::forward<Args>(args)...);
        }
    constexpr auto operator()(Tag tag, Args &&...args) const
        noexcept(noexcept(alpaqa_tag_invoke(std::move(tag),
                                            std::forward<Args>(args)...)))
            -> decltype(alpaqa_tag_invoke(std::move(tag),
                                          std::forward<Args>(args)...)) {
        return alpaqa_tag_invoke(std::move(tag), std::forward<Args>(args)...);
    }
};
} // namespace tag_invoke_fn_ns

// Function object needs to be in a separate namespace to avoid conflicts with
// hidden-friend customizations defined for types in the alpaqa namespace.
inline namespace tag_invoke_ns {
inline constexpr tag_invoke_fn_ns::tag_invoke_fn alpaqa_tag_invoke = {};
}

template <typename Tag, typename... Args>
concept tag_invocable = requires(Tag tag, Args... args) {
    alpaqa_tag_invoke(std::forward<Tag>(tag), std::forward<Args>(args)...);
};

template <typename Tag, typename... Args>
concept nothrow_tag_invocable =
    tag_invocable<Tag, Args...> && requires(Tag tag, Args... args) {
        {
            alpaqa_tag_invoke(std::forward<Tag>(tag),
                              std::forward<Args>(args)...)
        } noexcept;
    };

template <typename Tag, typename... Args>
inline constexpr bool is_tag_invocable_v = tag_invocable<Tag, Args...>;

template <typename Tag, typename... Args>
inline constexpr bool is_nothrow_tag_invocable_v =
    nothrow_tag_invocable<Tag, Args...>;

template <typename Tag, typename... Args>
using tag_invoke_result =
    std::invoke_result<decltype(::alpaqa::alpaqa_tag_invoke), Tag, Args...>;

template <typename Tag, typename... Args>
using tag_invoke_result_t =
    std::invoke_result_t<decltype(::alpaqa::alpaqa_tag_invoke), Tag, Args...>;

template <auto &Tag>
using tag_t = std::decay_t<decltype(Tag)>;

} // namespace alpaqa