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
template <class T>
struct attribute_accessor<T, ParamString> {
    template <class T_actual, class A>
    attribute_accessor(A T_actual::*attr)
        : set([attr](T &t, const ParamString &s) {
              return set_param(t.*attr, s);
          }) {}
    std::function<void(T &, const ParamString &)> set;
};

/// Use @p s to index into the struct type @p T and overwrite the attribute
/// given by @p s.key.
template <class T>
    requires requires { attribute_table<T, ParamString>::table; }
void set_param(T &t, ParamString s) {
    const auto &m         = attribute_table<T, ParamString>::table;
    auto [key, remainder] = split_key(s.key);
    auto it               = m.find(key);
    if (it == m.end()) {
        auto keys = std::views::keys(m);
        std::vector<std::string> sorted_keys{keys.begin(), keys.end()};
        util::sort_case_insensitive(sorted_keys);
        throw invalid_param(
            "Invalid key '" + std::string(key) + "' for type '" +
            demangled_typename(typeid(T)) + "' in '" + std::string(s.full_key) +
            "',\n  possible keys are: " +
            util::join(sorted_keys, {.sep = ", ", .empty = "∅"}));
    }
    s.key = remainder;
    it->second.set(t, s);
}

} // namespace alpaqa::params