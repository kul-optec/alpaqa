#include <gtest/gtest.h>

#include <alpaqa/config/config.hpp>
#include <alpaqa/util/print.hpp>
#include <sstream>

USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);

TEST(Print, pythonVector) {
    std::ostringstream ss;
    vec v(6);
    v << 1, 2, 3, 4, 5, 0.1;
    alpaqa::print_python(ss, v);
    EXPECT_EQ(ss.str(),
              "[+1.00000000000000000e+00, +2.00000000000000000e+00, "
              "+3.00000000000000000e+00, +4.00000000000000000e+00, "
              "+5.00000000000000000e+00, +1.00000000000000006e-01]\n");
}

TEST(Print, matlabVector) {
    std::ostringstream ss;
    vec v(6);
    v << 1, 2, 3, 4, 5, 0.1;
    alpaqa::print_matlab(ss, v);
    EXPECT_EQ(ss.str(),
              "[+1.00000000000000000e+00 +2.00000000000000000e+00 "
              "+3.00000000000000000e+00 +4.00000000000000000e+00 "
              "+5.00000000000000000e+00 +1.00000000000000006e-01];\n");
}

TEST(Print, pythonMatrix) {
    std::ostringstream ss;
    mat m(3, 2);
    m << 1, 2, 3, 4, 5, 0.1;
    alpaqa::print_python(ss, m);
    EXPECT_EQ(ss.str(),
              "[[+1.00000000000000000e+00, +2.00000000000000000e+00],\n"
              " [+3.00000000000000000e+00, +4.00000000000000000e+00],\n"
              " [+5.00000000000000000e+00, +1.00000000000000006e-01]]\n");
}

TEST(Print, matlabMatrix) {
    std::ostringstream ss;
    mat m(3, 2);
    m << 1, 2, 3, 4, 5, 0.1;
    alpaqa::print_matlab(ss, m);
    EXPECT_EQ(ss.str(),
              "[+1.00000000000000000e+00 +2.00000000000000000e+00;\n"
              " +3.00000000000000000e+00 +4.00000000000000000e+00;\n"
              " +5.00000000000000000e+00 +1.00000000000000006e-01];\n");
}

TEST(Print, pythonVectorInt) {
    std::ostringstream ss;
    Eigen::VectorX<int> v(6);
    v << 1, 2, 3, 4, 5, -100;
    alpaqa::print_python(ss, v);
    EXPECT_EQ(ss.str(), "[1, 2, 3, 4, 5, -100]\n");
}

TEST(Print, matlabVectorInt) {
    std::ostringstream ss;
    Eigen::VectorX<int> v(6);
    v << 1, 2, 3, 4, 5, -100;
    alpaqa::print_matlab(ss, v);
    EXPECT_EQ(ss.str(), "[1 2 3 4 5 -100];\n");
}

TEST(Print, pythonVectorIndex) {
    std::ostringstream ss;
    indexvec v(6);
    v << 1, 2, 3, 4, 5, -100;
    alpaqa::print_python(ss, v);
    EXPECT_EQ(ss.str(), "[1, 2, 3, 4, 5, -100]\n");
}

TEST(Print, matlabVectorIndex) {
    std::ostringstream ss;
    indexvec v(6);
    v << 1, 2, 3, 4, 5, -100;
    alpaqa::print_matlab(ss, v);
    EXPECT_EQ(ss.str(), "[1 2 3 4 5 -100];\n");
}