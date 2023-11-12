#pragma once

#include <alpaqa/util/demangled-typename.hpp>
#include <alpaqa/util/from_chars-wrapper.hpp>
#include <chrono>
#include <string_view>

namespace alpaqa::util {

struct ALPAQA_EXPORT invalid_duration_value : std::invalid_argument {
    explicit invalid_duration_value(const std::string &arg,
                                    std::from_chars_result result)
        : std::invalid_argument{arg}, result{result} {}
    std::from_chars_result result;
};

struct ALPAQA_EXPORT invalid_duration_units : std::invalid_argument {
    explicit invalid_duration_units(const std::string &arg,
                                    std::string_view units)
        : std::invalid_argument{arg}, units{units} {}
    /// Points into original argument, beware lifetime issues.
    std::string_view units;
};

/// Adds the first duration in the string @p s to the duration @p t.
template <class Rep, class Period>
std::string_view parse_single_duration(std::chrono::duration<Rep, Period> &t,
                                       std::string_view s) {
    using Duration = std::remove_cvref_t<decltype(t)>;
    auto trim      = s.find_first_not_of("+0 ");
    if (trim == std::string_view::npos)
        return {};
    s.remove_prefix(trim);
    const auto *val_end = s.data() + s.size();
    double value;
    auto res = from_chars(s.data(), val_end, value);
    if (res.ec != std::errc())
        throw invalid_duration_value(
            "Invalid value '" + std::string(res.ptr, val_end) + "' for type '" +
                demangled_typename(typeid(Duration)) +
                "': " + std::make_error_code(res.ec).message(),
            res);
    std::string_view remainder{res.ptr, val_end};
    auto end               = remainder.find_first_of("+-0123456789. ");
    std::string_view units = remainder.substr(0, end);
    using std::chrono::duration;
    auto cast = [](auto t) { return std::chrono::round<Duration>(t); };
    if (units == "s" || units.empty())
        t += cast(duration<double, std::ratio<1, 1>>{value});
    else if (units == "ms")
        t += cast(duration<double, std::ratio<1, 1'000>>{value});
    else if (units == "us" || units == "µs")
        t += cast(duration<double, std::ratio<1, 1'000'000>>{value});
    else if (units == "ns")
        t += cast(duration<double, std::ratio<1, 1'000'000'000>>{value});
    else if (units == "min")
        t += cast(duration<double, std::ratio<60, 1>>{value});
    else if (units == "h")
        t += cast(duration<double, std::ratio<3'600, 1>>{value});
    else
        throw invalid_duration_units(
            "Invalid units '" + std::string(units) + "' for duration", units);
    if (end == std::string_view::npos)
        return {};
    return remainder.substr(end);
}

/// Adds the sum of the durations in the string @p s to the duration @p t.
template <class Rep, class Period>
void parse_duration(std::chrono::duration<Rep, Period> &t, std::string_view s) {
    while (!s.empty())
        s = parse_single_duration(t, s);
}

} // namespace alpaqa::util