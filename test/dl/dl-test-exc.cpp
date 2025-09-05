#include <alpaqa/dl/dl-problem.h>
#include <dl-test-exc/export.h>
#include <stdexcept>

#include "data.hpp"

extern "C" DL_TEST_EXC_EXPORT alpaqa_problem_register_t
register_alpaqa_problem(alpaqa_register_arg_t user_data) try {
    if (!user_data.data)
        throw std::invalid_argument("Missing user data");
    if (user_data.type != alpaqa_register_arg_std_any)
        throw std::invalid_argument("Invalid user data type");
    auto *init = any_cast<InitData>(static_cast<std::any *>(user_data.data));
    if (!init)
        throw std::invalid_argument("Incorrect init data type");
    std::string challenge = init->message;
    init->message         = "ack";
    throw dl_exception(challenge);
} catch (...) {
    return {.exception = new alpaqa_exception_ptr_t{std::current_exception()}};
}

extern "C" DL_TEST_EXC_EXPORT alpaqa_dl_abi_version_t
register_alpaqa_problem_version() {
    return ALPAQA_DL_ABI_VERSION;
}
