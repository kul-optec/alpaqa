#pragma once

#include <alpaqa/export.h>
#include <alpaqa/params/structs.hpp>
#include <nlohmann/json_fwd.hpp>
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
struct enum_accessor<T, json> {
    enum_accessor(T value, std::string_view = "") : value{value} {}
    T value;
};

/// Custom parameter parsing exception.
struct ALPAQA_EXPORT invalid_json_param : std::invalid_argument {
    using std::invalid_argument::invalid_argument;
    std::vector<std::string> backtrace;
};

} // namespace alpaqa::params