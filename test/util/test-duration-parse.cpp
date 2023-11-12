#include <gtest/gtest.h>

#include <alpaqa/util/duration-parse.hpp>

// split

TEST(durationparse, single0) {
    std::chrono::milliseconds v{};
    alpaqa::util::parse_duration(v, "0");
    EXPECT_EQ(v.count(), 0);
}

TEST(durationparse, single1) {
    std::chrono::milliseconds v{};
    alpaqa::util::parse_duration(v, "1234ms");
    EXPECT_EQ(v.count(), 1234);
}

TEST(durationparse, single2) {
    std::chrono::milliseconds v{};
    alpaqa::util::parse_duration(v, "1min");
    EXPECT_EQ(v.count(), 60'000);
}

TEST(durationparse, multiple) {
    std::chrono::milliseconds v{};
    alpaqa::util::parse_duration(v, "1min12s13ms");
    EXPECT_EQ(v.count(), 72'013);
}

TEST(durationparse, multipleSpace) {
    std::chrono::milliseconds v{};
    alpaqa::util::parse_duration(v, "1min 12s 13ms");
    EXPECT_EQ(v.count(), 72'013);
}

TEST(durationparse, multiplePlus) {
    std::chrono::milliseconds v{};
    alpaqa::util::parse_duration(v, "1min+12s+13ms");
    EXPECT_EQ(v.count(), 72'013);
}

TEST(durationparse, trailingSpace) {
    std::chrono::milliseconds v{};
    alpaqa::util::parse_duration(v, "1min    ");
    EXPECT_EQ(v.count(), 60'000);
}

TEST(durationparse, multipleFloat) {
    std::chrono::milliseconds v{};
    alpaqa::util::parse_duration(v, "1.5min+12.5s+13ms");
    EXPECT_EQ(v.count(), 102'513);
}

TEST(durationparse, round) {
    std::chrono::milliseconds v{};
    alpaqa::util::parse_duration(v, "501µs");
    EXPECT_EQ(v.count(), 1);
}

TEST(durationparse, roundFloat) {
    std::chrono::milliseconds v{};
    alpaqa::util::parse_duration(v, "0.501ms");
    EXPECT_EQ(v.count(), 1);
}
