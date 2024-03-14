#pragma once

#include <alpaqa/params/structs.hpp>

#include <optional>
#include <string_view>
#include <vector>
using namespace std::string_view_literals;

namespace alpaqa::params {

struct MemberGetter {
    /// Full key string, used for diagnostics.
    std::string_view full_key;
    /// The subkey to resolve next.
    std::string_view key;
    /// The value of the parameter to store.
    std::optional<std::string_view> value;
};

struct Result {
    bool leaf;
    std::string_view prefix;
    struct Member {
        std::string_view name;
        std::optional<std::string_view> doc = std::nullopt;
        std::optional<char> suffix          = std::nullopt;
    };
    std::vector<Member> members;
};

template <class T>
Result get_members(const MemberGetter &s); /* deliberately undefined */

template <class T>
bool is_leaf(); /* deliberately undefined */

template <>
struct attribute_accessor<MemberGetter> {
    template <class T, class T_actual, class A>
    static attribute_accessor make(A T_actual::*, std::string_view doc = "") {
        return {
            .get{[](const auto &s) { return get_members<A>(s); }},
            .doc{doc},
            .leaf{is_leaf<A>()},
        };
    }

    using get_members_func_t = Result(const MemberGetter &);
    get_members_func_t *get;
    std::string_view doc;
    bool leaf;
};

template <class T>
struct enum_accessor<T, MemberGetter> {
    enum_accessor(T, std::string_view doc = "") : doc(doc) {}
    std::string_view doc;
};

} // namespace alpaqa::params

void print_completion(std::string_view method, std::string_view params);
