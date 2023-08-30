#pragma once

#include <stdexcept>

namespace alpaqa::cutest {

struct function_load_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

enum class Status {
    Success  = 0, ///< Successful call.
    AllocErr = 1, ///< Array allocation/deallocation error.
    BoundErr = 2, ///< Array bound error.
    EvalErr  = 3, ///< Evaluation error.
};

inline constexpr const char *enum_name(Status s) {
    switch (s) {
        case Status::Success: return "Success";
        case Status::AllocErr: return "AllocErr";
        case Status::BoundErr: return "BoundErr";
        case Status::EvalErr: return "EvalErr";
        default:;
    }
    return "<unknown>";
}

struct function_call_error : std::runtime_error {
    function_call_error(std::string message, Status status)
        : runtime_error{std::move(message) + ": " + enum_name(status) + " (" +
                        std::to_string(static_cast<int>(status)) + ')'},
          status{status} {}
    Status status;
};

} // namespace alpaqa::cutest