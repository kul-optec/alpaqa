#include <alpaqa/params/params.hpp>

#include <alpaqa/config/config.hpp>
#include <alpaqa/params/structs.hpp>
#include <alpaqa/util/demangled-typename.hpp>
#include <alpaqa/util/string-util.hpp>

namespace alpaqa::params {

using config_t = DefaultConfig;

/// Throw a meaningful error when `s.key` is not empty, to indicate that
/// the given type @p T is not of struct type and cannot be indexed into.
template <class T>
void assert_key_empty(ParamString s) {
    if (!s.key.empty())
        throw invalid_param("Type '" + demangled_typename(typeid(T)) +
                            "' cannot be indexed in '" +
                            std::string(s.full_key) + "'");
}

/// Throw a meaningful error to indicate that parameters of type @p T are not
/// supported or implemented.
template <class T>
void unsupported_type(T &, [[maybe_unused]] ParamString s) {
    throw invalid_param("Unknown parameter type '" +
                        demangled_typename(typeid(T)) + "' in '" +
                        std::string(s.full_key) + "'");
}

/// Function wrapper to set attributes of a struct, type-erasing the type of the
/// attribute.
template <>
struct attribute_accessor<ParamString> {
    template <class T, class T_actual, class A>
    static attribute_accessor make(A T_actual::*attr, std::string_view = "") {
        return {{[attr](const any_ptr &t, const ParamString &s) {
            return set_param(t.template cast<T>()->*attr, s);
        }}};
    }
    std::function<void(const any_ptr &, const ParamString &)> set;
};

template <class T>
struct enum_accessor<T, ParamString> {
    enum_accessor(T value, std::string_view = "") : value{value} {}
    T value;
};

namespace detail {
template <class S>
auto find_param(const attribute_table_t<S> &m, std::string_view key,
                std::string &error_msg)
    -> std::optional<typename attribute_table_t<S>::const_iterator> {
    auto it = m.find(key);
    if (it == m.end()) {
        auto keys = std::views::keys(m);
        std::vector<std::string> sorted_keys{keys.begin(), keys.end()};
        util::sort_case_insensitive(sorted_keys);
        error_msg = util::join(sorted_keys, {.sep = ", ", .empty = "∅"});
        return std::nullopt;
    }
    return std::make_optional(it);
}
} // namespace detail

/// Use @p s to index into the struct type @p T and overwrite the attribute
/// given by @p s.key.
template <class T>
    requires requires { attribute_table<T, ParamString>::table; }
void set_param_default(T &t, ParamString s) {
    const auto &m         = attribute_table<T, ParamString>::table;
    auto [key, remainder] = split_key(s.key);
    std::string error_msg;
    auto param = detail::find_param(m, key, error_msg);
    if (!param)
        throw invalid_param("Invalid key '" + std::string(key) +
                            "' for type '" + demangled_typename(typeid(T)) +
                            "' in '" + std::string(s.full_key) +
                            "',\n  possible keys are: " + error_msg);
    s.key = remainder;
    (*param)->second.set(&t, s);
}

/// Set @p t to the value of @p s.value.
template <class T>
    requires requires { enum_table<T, ParamString>::table; }
void set_param_default(T &t, ParamString s) {
    assert_key_empty<T>(s);
    const auto &m = enum_table<T, ParamString>::table;
    auto it       = m.find(s.value);
    if (it == m.end()) {
        auto vals = std::views::keys(m);
        std::vector<std::string> sorted_vals{vals.begin(), vals.end()};
        util::sort_case_insensitive(sorted_vals);
        throw invalid_param(
            "Invalid value '" + std::string(s.value) + "' for enum '" +
            demangled_typename(typeid(T)) + "' in '" + std::string(s.full_key) +
            "',\n  possible value are: " +
            util::join(sorted_vals, {.sep = ", ", .empty = "∅"}));
    }
    t = it->second.value;
}

} // namespace alpaqa::params