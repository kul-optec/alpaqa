#pragma once

#include <algorithm>
#include <numeric>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>

namespace alpaqa::util {

/// Split the string @p full on the first occurrence of @p tok.
/// Returns `(s, "")` if tok was not found.
inline auto split(std::string_view full, std::string_view tok) {
    auto tok_pos = full.find(tok);
    if (tok_pos == full.npos) {
        std::string_view key = full;
        std::string_view rem{key.data() + key.size(), 0};
        return std::make_tuple(key, rem);
    } else {
        auto tok_len         = tok.size();
        std::string_view key = full.substr(0, tok_pos);
        std::string_view rem = full.substr(tok_pos + tok_len);
        return std::make_tuple(key, rem);
    }
}

/// Split the string @p s on the first occurrence of @p tok.
/// Returns `("", s)` if tok was not found.
inline auto split_second(std::string_view full, std::string_view tok) {
    auto tok_pos = full.find(tok);
    if (tok_pos == full.npos) {
        std::string_view key{full.data(), 0};
        std::string_view rem = full;
        return std::make_tuple(key, rem);
    } else {
        auto tok_len         = tok.size();
        std::string_view key = full.substr(0, tok_pos);
        std::string_view rem = full.substr(tok_pos + tok_len);
        return std::make_tuple(key, rem);
    }
}

/// @see @ref join
struct join_opt {
    std::string_view sep   = ", ";
    std::string_view empty = "∅";
};

/// Join the list of strings into a single string, using the separator given by
/// @p opt.
/// @see @ref join_opt
std::string join(std::ranges::input_range auto strings, join_opt opt = {}) {
    if (std::ranges::empty(strings))
        return std::string(opt.empty);
    auto combine = [&opt](std::string &&acc, const auto &e) {
        acc += opt.sep;
        acc += e;
        return std::move(acc);
    };
    auto begin = std::ranges::begin(strings);
    auto end   = std::ranges::end(strings);
    using std::ranges::next;
    std::string first{*begin};
    return std::accumulate(next(begin), end, std::move(first), combine);
}

/// @see @ref join_quote
struct join_quote_opt {
    std::string_view sep         = ", ";
    std::string_view empty       = "∅";
    std::string_view quote_left  = "\"";
    std::string_view quote_right = "\"";
};

/// Join the list of strings into a single string, using the separator given by
/// @p opt. Each original string is quoted using the quote strings specified
/// by @p opt
/// @see @ref join_quote_opt
std::string join_quote(std::ranges::input_range auto strings,
                       join_quote_opt opt = {}) {
    if (std::ranges::empty(strings))
        return std::string(opt.empty);
    auto combine = [&opt](std::string &&acc, const auto &e) {
        acc += opt.quote_right;
        acc += opt.sep;
        acc += opt.quote_left;
        acc += e;
        return std::move(acc);
    };
    auto begin = std::ranges::begin(strings);
    auto end   = std::ranges::end(strings);
    std::string first{*begin};
    first.insert(0, opt.quote_left);
    using std::ranges::next;
    auto result = std::accumulate(next(begin), end, std::move(first), combine);
    result += opt.quote_right;
    return result;
}

/// Sort the given range of strings in-place in a case-insensitive manner.
void sort_case_insensitive(auto &range) {
    auto cmp = [](const auto &a, const auto &b) {
        auto toupper = [](unsigned char c) { return std::toupper(c); };
        return std::ranges::lexicographical_compare(
            std::views::transform(a, toupper),
            std::views::transform(b, toupper));
    };
    std::ranges::sort(range, cmp);
}

} // namespace alpaqa::util