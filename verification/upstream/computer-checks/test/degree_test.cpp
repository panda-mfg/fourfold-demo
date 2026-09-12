#include "common.hpp"

TEST(DegreeTest, Compare) {
    Degree d1(5, 6);
    Degree d2(5, 6);
    Degree d3(5, 7);
    Degree d4(6, 7);
    EXPECT_EQ(d1, d2);
    EXPECT_LT(d1, d3);
    EXPECT_LT(d1, d4);
    EXPECT_GT(d3, d1);
    EXPECT_GT(d4, d1);
}