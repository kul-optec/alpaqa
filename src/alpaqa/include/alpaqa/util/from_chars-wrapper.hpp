#pragma once

#include <charconv>
#include <stdexcept>
#include <system_error>

#if __cpp_lib_to_chars

#define ALPAQA_USE_FROM_CHARS_INT 1
#if defined(__clang__) // Clang
#pragma message("Using std::stod as a fallback to replace std::from_chars")
#define ALPAQA_USE_FROM_CHARS_FLOAT 0
#elif defined(_MSC_VER) // MSVC
#if _MSC_VER >= 1924
#define ALPAQA_USE_FROM_CHARS_FLOAT 1
#else
#pragma message("Using std::stod as a fallback to replace std::from_chars")
#define ALPAQA_USE_FROM_CHARS_FLOAT 0
#endif
#elif defined(__GNUC__) // GCC
#if __GNUC__ >= 11
#define ALPAQA_USE_FROM_CHARS_FLOAT 1
#else
#pragma message("Using std::stod as a fallback to replace std::from_chars")
#define ALPAQA_USE_FROM_CHARS_FLOAT 0
#endif
#else // Unknown
#pragma message("Unknown compiler: not using std::from_chars for floats")
#define ALPAQA_USE_FROM_CHARS_FLOAT 0
#endif

#else // __cpp_lib_to_chars
#define ALPAQA_USE_FROM_CHARS_INT 0
#define ALPAQA_USE_FROM_CHARS_FLOAT 0
#endif

namespace alpaqa::util {

template <class T>
    requires(
#if ALPAQA_USE_FROM_CHARS_FLOAT
        std::floating_point<T> ||
#endif
        false) // NOLINT(readability-simplify-boolean-expr)
std::from_chars_result
from_chars(const char *first, const char *last, T &value,
           std::chars_format fmt = std::chars_format::general) {
    return std::from_chars(first, last, value, fmt);
}

template <class T>
    requires(
#if ALPAQA_USE_FROM_CHARS_INT
        std::integral<T> ||
#endif
        false) // NOLINT(readability-simplify-boolean-expr)
std::from_chars_result from_chars(const char *first, const char *last, T &value,
                                  int base = 10) {
    return std::from_chars(first, last, value, base);
}

template <class T, class... Args>
    requires(
#if !ALPAQA_USE_FROM_CHARS_FLOAT
        std::floating_point<T> ||
#endif
        false) // NOLINT(readability-simplify-boolean-expr)
std::from_chars_result from_chars(
    const char *first, const char *last, T &value,
    [[maybe_unused]] std::chars_format fmt = std::chars_format::general) {
    size_t end_index = 0;
    try {
        if constexpr (std::is_same_v<T, float>)
            value = std::stof(std::string(first, last), &end_index);
        else if constexpr (std::is_same_v<T, double>)
            value = std::stod(std::string(first, last), &end_index);
        else if constexpr (std::is_same_v<T, long double>)
            value = std::stold(std::string(first, last), &end_index);
        else
            static_assert(std::is_same_v<T, void>); // false
    } catch (std::invalid_argument &e) {
        return {
            .ptr = first,
            .ec  = std::errc::invalid_argument,
        };
    } catch (std::out_of_range &e) {
        return {
            .ptr = first + end_index,
            .ec  = std::errc::result_out_of_range,
        };
    }
    return {
        .ptr = first + end_index,
        .ec  = std::errc(),
    };
}

template <class T, class... Args>
    requires(
#if !ALPAQA_USE_FROM_CHARS_INT
        std::integral<T> ||
#endif
        false) // NOLINT(readability-simplify-boolean-expr)
std::from_chars_result from_chars(const char *first, const char *last, T &value,
                                  int base = 10) {
    size_t end_index = 0;
    try {
        if constexpr (std::is_same_v<T, signed char>)
            value = static_cast<signed char>(
                std::stoi(std::string(first, last), &end_index, base));
        else if constexpr (std::is_same_v<T, short>)
            value = static_cast<short>(
                std::stoi(std::string(first, last), &end_index, base));
        else if constexpr (std::is_same_v<T, int>)
            value = std::stoi(std::string(first, last), &end_index, base);
        else if constexpr (std::is_same_v<T, long>)
            value = std::stol(std::string(first, last), &end_index, base);
        else if constexpr (std::is_same_v<T, long long>)
            value = std::stoll(std::string(first, last), &end_index, base);
        else if constexpr (std::is_same_v<T, unsigned char>)
            value = static_cast<unsigned char>(
                std::stoul(std::string(first, last), &end_index, base));
        else if constexpr (std::is_same_v<T, unsigned short>)
            value = static_cast<unsigned short>(
                std::stoul(std::string(first, last), &end_index, base));
        else if constexpr (std::is_same_v<T, unsigned int>)
            value = static_cast<unsigned int>(
                std::stoul(std::string(first, last), &end_index, base));
        else if constexpr (std::is_same_v<T, unsigned long>)
            value = std::stoul(std::string(first, last), &end_index, base);
        else if constexpr (std::is_same_v<T, unsigned long long>)
            value = std::stoull(std::string(first, last), &end_index, base);
        else
            static_assert(std::is_same_v<T, void>); // false
    } catch (std::invalid_argument &e) {
        return {
            .ptr = first,
            .ec  = std::errc::invalid_argument,
        };
    } catch (std::out_of_range &e) {
        return {
            .ptr = first + end_index,
            .ec  = std::errc::result_out_of_range,
        };
    }
    return {
        .ptr = first + end_index,
        .ec  = std::errc(),
    };
}
} // namespace alpaqa::util