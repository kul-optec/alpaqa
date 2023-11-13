#pragma once

#include <alpaqa/export.h>
#include <alpaqa/params/structs.hpp>
#include <nlohmann/json_fwd.hpp>
#include <functional>
#include <string>
#include <vector>

namespace alpaqa::params {

using nlohmann::json;

/// Update/overwrite the first argument based on the JSON object @p j.
template <class T>
void ALPAQA_EXPORT set_param(T &, const json &j); /* deliberately undefined */

/// Get the first argument as a JSON object @p j.
template <class T>
void ALPAQA_EXPORT get_param(const T &, json &j); /* deliberately undefined */

template <class T>
struct attribute_accessor<T, json> {
    template <class T_actual, class A>
    attribute_accessor(A T_actual::*attr)
        : set([attr](T &t, const json &s) { return set_param(t.*attr, s); }),
          get([attr](const T &t, json &s) { return get_param(t.*attr, s); }) {}
    std::function<void(T &, const json &)> set;
    std::function<void(const T &, json &)> get;
};

/// Custom parameter parsing exception.
struct ALPAQA_EXPORT invalid_json_param : std::invalid_argument {
    using std::invalid_argument::invalid_argument;
    std::vector<std::string> backtrace;
};

} // namespace alpaqa::params