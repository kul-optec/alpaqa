#include <alpaqa/config/config.hpp>
#include <alpaqa/params/json.hpp>
#include <alpaqa/params/params.hpp>
#include <alpaqa/util/demangled-typename.hpp>
#include <alpaqa/util/string-util.hpp>

#include <nlohmann/json.hpp>

namespace alpaqa::params {

using config_t = DefaultConfig;

template <class T>
    requires requires { attribute_table<T, nlohmann::json>::table; }
void set_param(T &t, const nlohmann::json &j) {
    if (!j.is_object())
        throw std::runtime_error("should be object");
    const auto &m = attribute_table<T, nlohmann::json>::table;
    for (auto &&el : j.items()) {
        const auto &key = el.key();
        auto it         = m.find(key);
        if (it == m.end()) {
            auto keys = std::views::keys(m);
            std::vector<std::string> sorted_keys{keys.begin(), keys.end()};
            util::sort_case_insensitive(sorted_keys);
            throw invalid_json_param(
                "Invalid key '" + key + "' for type '" +
                demangled_typename(typeid(T)) + "',\n  possible keys are: " +
                util::join(sorted_keys, {.sep = ", ", .empty = "∅"}));
        }
        it->second.set(t, el.value());
    }
}

template <class T>
    requires requires { attribute_table<T, nlohmann::json>::table; }
void get_param(const T &t, nlohmann::json &s) {
    s             = nlohmann::json::object();
    const auto &m = attribute_table<T, nlohmann::json>::table;
    for (auto &&[k, v] : m)
        v.get(t, s[k]);
}

} // namespace alpaqa::params