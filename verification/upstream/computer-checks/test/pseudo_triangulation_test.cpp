#include "../src/pseudo_triangulation.hpp"
#include <gtest/gtest.h>
using std::vector;

TEST(PseudoTriangulationTest, FromVRotation) {
    int N                        = 3;
    vector<vector<int>> rotation = {{1, 2, -1}, {2, 0, -1}, {0, 1, -1}};
    PseudoTriangulation pt       = PseudoTriangulation::from_v_rotations(N, rotation);
    PseudoTriangulation pt_expected =
        PseudoTriangulation(3, {Dart(0, 3, 1, -1), Dart(0, 4, -1, 0), Dart(1, 5, 3, -1),
                                Dart(1, 0, -1, 2), Dart(2, 1, 5, -1), Dart(2, 2, -1, 4)});
    EXPECT_EQ(pt, pt_expected);
}

TEST(PseudoTriangulationTest, Identify) {
    vector<vector<int>> rotation0 = {{1, 2, -1}, {2, 0, -1}, {0, 1, -1}};
    PseudoTriangulation pt0       = PseudoTriangulation::from_v_rotations(3, rotation0);
    PseudoTriangulation pt1       = PseudoTriangulation::from_v_rotations(3, rotation0);
    auto [pt, mappings0, mappings1] =
        PseudoTriangulation::free_homomorphism(pt0, pt1, 0, 5); // identify (0,1) and (2,1)
    PseudoTriangulation pt_expected = PseudoTriangulation(
        4, {Dart(3, 2, -1, 9), Dart(2, 3, 6, -1), Dart(0, 0, 3, -1), Dart(0, 1, -1, 2),
            Dart(1, 7, 5, -1), Dart(1, 8, -1, 4), Dart(2, 9, 7, 1), Dart(2, 4, -1, 6),
            Dart(3, 5, 9, -1), Dart(3, 6, 0, 8)});
    vector<int> vmap0_expected = {3, 2, 0};
    vector<int> vmap1_expected = {1, 2, 3};
    vector<int> dmap0_expected = {9, 0, 1, 6, 2, 3};
    vector<int> dmap1_expected = {4, 5, 6, 7, 8, 9};
    EXPECT_EQ(mappings0.vmap, vmap0_expected);
    EXPECT_EQ(mappings1.vmap, vmap1_expected);
    EXPECT_EQ(mappings0.dmap, dmap0_expected);
    EXPECT_EQ(mappings1.dmap, dmap1_expected);
    EXPECT_EQ(pt, pt_expected);
}

TEST(PseudoTriangulationTest, Identify2) {
    // 5-wheel + one vertex
    PseudoTriangulation pt0 = PseudoTriangulation::from_v_rotations(7, {{1, 2, 3, 4, 5},
                                                                        {2, 0, 5, -1},
                                                                        {3, 0, 1, -1},
                                                                        {4, 0, 2, -1},
                                                                        {6, 5, 0, 3, -1},
                                                                        {1, 0, 4, 6, -1},
                                                                        {5, 4, -1}});
    // 7-wheel + one vertex
    PseudoTriangulation pt1 = PseudoTriangulation::from_v_rotations(9, {{1, 2, 3, 4, 5, 6, 7},
                                                                        {8, 2, 0, 7, -1},
                                                                        {3, 0, 1, 8, -1},
                                                                        {4, 0, 2, -1},
                                                                        {5, 0, 3, -1},
                                                                        {6, 0, 4, -1},
                                                                        {7, 0, 5, -1},
                                                                        {1, 0, 6, -1},
                                                                        {2, 1, -1}});
    auto [pt, mappings0, mappings1] =
        PseudoTriangulation::free_homomorphism(pt0, pt1, 7, 10); // identify (1,5) and (1,7)
    PseudoTriangulation pt_expected = PseudoTriangulation(
        3, {Dart(1, 1, 4, -1), Dart(2, 0, -1, 7), Dart(0, 5, 2, 2), Dart(1, 7, -1, 6),
            Dart(1, 6, 5, 0), Dart(1, 2, 6, 4), Dart(1, 4, 3, 5), Dart(2, 3, 1, -1)});
    EXPECT_EQ(pt, pt_expected);
}
