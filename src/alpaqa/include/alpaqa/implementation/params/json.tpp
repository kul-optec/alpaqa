#include <alpaqa/config/config.hpp>
#include <alpaqa/params/json.hpp>
#include <alpaqa/util/demangled-typename.hpp>
#include <alpaqa/util/string-util.hpp>

#include <nlohmann/json.hpp>
#include <functional>
#include <stdexcept>
#include <type_traits>

namespace alpaqa::params {

using config_t = DefaultConfig;

namespace detail {

std::string join_sorted_keys(const auto &members) {
    auto keys = std::views::keys(members);
    std::vector<std::string> sorted_keys{keys.begin(), keys.end()};
    util::sort_case_insensitive(sorted_keys);
    return util::join(sorted_keys, {.sep = ", ", .empty = "∅"});
}

template <class S>
[[gnu::noinline]] auto
find_param_or_throw(const std::string &key, const attribute_table_t<S> &members,
                    const attribute_alias_table_t<S> &aliases,
                    const std::string &type_name) ->
    typename attribute_table_t<S>::const_iterator {
    auto it = members.find(key);
    // If member was not found
    if (it == members.end()) {
        // Perhaps it's an alias to another member?
        auto alias_it = aliases.find(key);
        // If it's not an alias either, raise an error
        if (alias_it == aliases.end()) {
            throw invalid_json_param(
                "Invalid key '" + key + "' for type '" + type_name +
                "',\n  possible keys are: " + join_sorted_keys(members) +
                " (aliases: " + join_sorted_keys(aliases) + ")");
        }
        // Resolve the alias and make sure that the target exists
        it = members.find(alias_it->second);
        if (it == members.end())
            throw std::logic_error("Alias '" + std::string(alias_it->first) +
                                   "' refers to nonexistent option '" +
                                   std::string(alias_it->second) + "' in '" +
                                   type_name + "'");
    }
    return it;
}

template <class S>
[[gnu::noinline]] auto
find_param_or_throw(const std::string &key, const attribute_table_t<S> &members,
                    const std::false_type &, const std::string &type_name) ->
    typename attribute_table_t<S>::const_iterator {
    auto it = members.find(key);
    // If member was not found
    if (it == members.end()) {
        throw invalid_json_param(
            "Invalid key '" + key + "' for type '" + type_name +
            "',\n  possible keys are: " + join_sorted_keys(members));
    }
    return it;
}

template <class Aliases>
[[gnu::noinline]] void set_param_json(const any_ptr &t, const json &j,
                                      const attribute_table_t<json> &members,
                                      const Aliases &aliases,
                                      const std::string &type_name) {
    if (!j.is_object())
        throw invalid_json_param(
            "Invalid value " + to_string(j) + " for type '" + type_name +
            "' (expected object, but got " + j.type_name() + ')');
    // Loop over all items in the JSON object
    for (auto &&el : j.items()) {
        const auto &key = el.key();
        auto it         = find_param_or_throw(key, members, aliases, type_name);
        // Member was found, invoke its setter (and possibly recurse)
        try {
            it->second.set(t, el.value());
        } catch (invalid_json_param &e) {
            // Keep a backtrace of the JSON keys for error reporting
            e.backtrace.push_back(key);
            throw;
        }
    }
}

} // namespace detail

template <>
struct attribute_accessor<json> {
    template <class T, class T_actual, class A>
    static attribute_accessor make(A T_actual::*attr, std::string_view = "") {
        return {
            .set{[attr](const any_ptr &t, const json &s) {
                return set_param(t.template cast<T>()->*attr, s);
            }},
            .get{[attr](const any_ptr &t, json &s) {
                return get_param(t.template cast<const T>()->*attr, s);
            }},
        };
    }
    std::function<void(const any_ptr &, const json &)> set;
    std::function<void(const any_ptr &, json &)> get;
};

template <class T>
    requires requires { attribute_table<T, json>::table; }
void set_param_default(T &t, const json &j) {
    // Dictionary of members
    const auto &members = attribute_table<T, json>::table;
    if constexpr (requires { attribute_alias_table<T, json>::table; })
        detail::set_param_json(&t, j, members,
                               attribute_alias_table<T, json>::table,
                               demangled_typename(typeid(T)));
    else
        detail::set_param_json(&t, j, members, std::false_type{},
                               demangled_typename(typeid(T)));
}

template <class T>
    requires requires { enum_table<T, json>::table; }
void set_param_default(T &t, const json &j) {
    if (!j.is_string())
        throw invalid_json_param("Invalid value " + to_string(j) +
                                 " for enum '" + demangled_typename(typeid(T)) +
                                 "' (expected string, but got " +
                                 j.type_name() + ')');
    // Dictionary of members
    const auto &m     = enum_table<T, json>::table;
    std::string value = j;
    auto it           = m.find(value);
    if (it == m.end()) {
        throw invalid_json_param(
            "Invalid value '" + value + "' for enum '" +
            demangled_typename(typeid(T)) +
            "',\n  possible values are: " + detail::join_sorted_keys(m));
    }
    t = it->second.value;
}

template <class T>
    requires requires { enum_table<T, json>::table; }
void get_param_default(const T &t, json &j) {
    j = enum_name(t);
}

template <class T>
    requires requires { attribute_table<T, json>::table; }
void get_param_default(const T &t, json &s) {
    s             = json::object();
    const auto &m = attribute_table<T, json>::table;
    for (auto &&[k, v] : m)
        v.get(&t, s[std::string{k}]);
}

} // namespace alpaqa::params