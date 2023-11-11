#include <gtest/gtest.h>

#include <alpaqa/util/string-util.hpp>

// split

TEST(stringutil, split) {
    using strvwtup = std::tuple<std::string_view, std::string_view>;
    EXPECT_EQ(alpaqa::util::split("abc.defgh", "."), strvwtup("abc", "defgh"));
}

TEST(stringutil, split2) {
    using strvwtup = std::tuple<std::string_view, std::string_view>;
    EXPECT_EQ(alpaqa::util::split("abc..e.", "."), strvwtup("abc", ".e."));
}

TEST(stringutil, splitEmpty) {
    using strvwtup = std::tuple<std::string_view, std::string_view>;
    EXPECT_EQ(alpaqa::util::split("abc", "."), strvwtup("abc", ""));
}

TEST(stringutil, splitStringview) {
    using strvwtup = std::tuple<std::string_view, std::string_view>;
    EXPECT_EQ(alpaqa::util::split("abcdefgh", "cde"), strvwtup("ab", "fgh"));
}

// split_second

TEST(stringutil, splitSecond) {
    using strvwtup = std::tuple<std::string_view, std::string_view>;
    EXPECT_EQ(alpaqa::util::split_second("abc.defgh", "."),
              strvwtup("abc", "defgh"));
}

TEST(stringutil, splitSecond2) {
    using strvwtup = std::tuple<std::string_view, std::string_view>;
    EXPECT_EQ(alpaqa::util::split_second("abc..e.", "."),
              strvwtup("abc", ".e."));
}

TEST(stringutil, splitSecondEmpty) {
    using strvwtup = std::tuple<std::string_view, std::string_view>;
    EXPECT_EQ(alpaqa::util::split_second("abc", "."), strvwtup("", "abc"));
}

TEST(stringutil, splitSecondStringview) {
    using strvwtup = std::tuple<std::string_view, std::string_view>;
    EXPECT_EQ(alpaqa::util::split_second("abcdefgh", "cde"),
              strvwtup("ab", "fgh"));
}

// join

TEST(stringutil, join) {
    std::array strings{"abc", "de", "fghi"};
    EXPECT_EQ(alpaqa::util::join(strings), "abc, de, fghi");
}

TEST(stringutil, join2) {
    std::array strings{"abc", "de", "fghi"};
    EXPECT_EQ(alpaqa::util::join(strings, {.sep = "<->"}), "abc<->de<->fghi");
}

TEST(stringutil, joinEmpty) {
    std::array<std::string_view, 0> strings;
    EXPECT_EQ(alpaqa::util::join(strings), "∅");
}

TEST(stringutil, joinEmpty2) {
    std::array<std::string_view, 0> strings;
    EXPECT_EQ(alpaqa::util::join(strings, {.empty = "[]"}), "[]");
}

// join_quote

TEST(stringutil, joinQuote) {
    std::array strings{"abc", "de", "fghi"};
    EXPECT_EQ(alpaqa::util::join_quote(strings), R"("abc", "de", "fghi")");
}

TEST(stringutil, joinQuote2) {
    std::array strings{"abc", "de", "fghi"};
    EXPECT_EQ(
        alpaqa::util::join_quote(
            strings, {.sep = " - ", .quote_left = "“", .quote_right = "”"}),
        "“abc” - “de” - “fghi”");
}

TEST(stringutil, joinQuoteEmpty) {
    std::array<std::string_view, 0> strings;
    EXPECT_EQ(alpaqa::util::join_quote(strings), "∅");
}

TEST(stringutil, joinQuoteEmpty2) {
    std::array<std::string_view, 0> strings;
    EXPECT_EQ(alpaqa::util::join_quote(strings, {.empty = "[]"}), "[]");
}
