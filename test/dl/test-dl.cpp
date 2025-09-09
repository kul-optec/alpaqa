#include <alpaqa/dl/dl-problem.hpp>
#include <gtest/gtest.h>

#include "data.hpp"

TEST(Dl, exception) {
    std::string challenge = "0xDEADBEEF";
    std::any init{InitData{.message = challenge}};
    try {
        alpaqa::dl::DLProblem problem(DL_TEST_EXC_DLL, "register_test_module",
                                      init, {.deepbind = false});
    } catch (dl_exception &e) {
        EXPECT_EQ(e.what(), challenge);
        EXPECT_EQ(any_cast<InitData>(init).message, "ack");
    }
}
