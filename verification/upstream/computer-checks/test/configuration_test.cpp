#include "common.hpp"

TEST_F(ConfFiles, ReadFile) {
    vector<Configuration> confs1   = Configuration::from_file(conffile1);
    vector<Configuration> confs2   = Configuration::from_file(conffile2);
    Configuration confs1_expected0 = Configuration(
        9, 8, {Dart(0, 22, 1, -1),  Dart(0, 10, 2, 0),   Dart(0, 18, -1, 1),  Dart(1, 7, 4, -1),
               Dart(1, 23, -1, 3),  Dart(2, 12, 6, -1),  Dart(2, 24, 7, 5),   Dart(2, 3, -1, 6),
               Dart(3, 15, 9, -1),  Dart(3, 19, 10, 8),  Dart(3, 1, 11, 9),   Dart(3, 25, 12, 10),
               Dart(3, 5, -1, 11),  Dart(4, 17, 14, -1), Dart(4, 20, 15, 13), Dart(4, 8, -1, 14),
               Dart(5, 21, 17, -1), Dart(5, 13, -1, 16), Dart(6, 2, 19, -1),  Dart(6, 9, 20, 18),
               Dart(6, 14, 21, 19), Dart(6, 16, -1, 20), Dart(7, 0, -1, 25),  Dart(7, 4, 24, -1),
               Dart(7, 6, 25, 23),  Dart(7, 11, 22, 24)},
        {Degree(4, INFTY), Degree(5), Degree(5), Degree(6), Degree(5), Degree(5), Degree(6),
         Degree(6)});
    Configuration confs1_expected1 = Configuration(
        11, 8, {Dart(0, 14, 1, -1),  Dart(0, 9, 2, 0),    Dart(0, 5, -1, 1),   Dart(1, 8, 4, -1),
                Dart(1, 23, -1, 3),  Dart(2, 2, 6, -1),   Dart(2, 13, 7, 5),   Dart(2, 24, 8, 6),
                Dart(2, 3, -1, 7),   Dart(3, 1, 10, 13),  Dart(3, 17, 11, 9),  Dart(3, 20, -1, 10),
                Dart(3, 25, 13, -1), Dart(3, 6, 9, 12),   Dart(4, 0, -1, 17),  Dart(4, 19, 16, -1),
                Dart(4, 21, 17, 15), Dart(4, 10, 14, 16), Dart(5, 22, 19, -1), Dart(5, 15, -1, 18),
                Dart(6, 11, 21, -1), Dart(6, 16, 22, 20), Dart(6, 18, -1, 21), Dart(7, 4, 24, -1),
                Dart(7, 7, 25, 23),  Dart(7, 12, -1, 24)},
        {Degree(4, INFTY), Degree(5), Degree(5), Degree(6), Degree(5), Degree(5), Degree(6),
         Degree(6)});
    Configuration confs1_expected2 = Configuration(
        9, 8, {Dart(0, 22, -1, 1),  Dart(0, 10, 0, 2),   Dart(0, 18, 1, -1),  Dart(1, 7, -1, 4),
               Dart(1, 23, 3, -1),  Dart(2, 12, -1, 6),  Dart(2, 24, 5, 7),   Dart(2, 3, 6, -1),
               Dart(3, 15, -1, 9),  Dart(3, 19, 8, 10),  Dart(3, 1, 9, 11),   Dart(3, 25, 10, 12),
               Dart(3, 5, 11, -1),  Dart(4, 17, -1, 14), Dart(4, 20, 13, 15), Dart(4, 8, 14, -1),
               Dart(5, 21, -1, 17), Dart(5, 13, 16, -1), Dart(6, 2, -1, 19),  Dart(6, 9, 18, 20),
               Dart(6, 14, 19, 21), Dart(6, 16, 20, -1), Dart(7, 0, 25, -1),  Dart(7, 4, -1, 24),
               Dart(7, 6, 23, 25),  Dart(7, 11, 24, 22)},
        {Degree(4, INFTY), Degree(5), Degree(5), Degree(6), Degree(5), Degree(5), Degree(6),
         Degree(6)});
    Configuration confs1_expected3 = Configuration(
        11, 8, {Dart(0, 14, -1, 1),  Dart(0, 9, 0, 2),    Dart(0, 5, 1, -1),   Dart(1, 8, -1, 4),
                Dart(1, 23, 3, -1),  Dart(2, 2, -1, 6),   Dart(2, 13, 5, 7),   Dart(2, 24, 6, 8),
                Dart(2, 3, 7, -1),   Dart(3, 1, 13, 10),  Dart(3, 17, 9, 11),  Dart(3, 20, 10, -1),
                Dart(3, 25, -1, 13), Dart(3, 6, 12, 9),   Dart(4, 0, 17, -1),  Dart(4, 19, -1, 16),
                Dart(4, 21, 15, 17), Dart(4, 10, 16, 14), Dart(5, 22, -1, 19), Dart(5, 15, 18, -1),
                Dart(6, 11, -1, 21), Dart(6, 16, 20, 22), Dart(6, 18, 21, -1), Dart(7, 4, -1, 24),
                Dart(7, 7, 23, 25),  Dart(7, 12, 24, -1)},
        {Degree(4, INFTY), Degree(5), Degree(5), Degree(6), Degree(5), Degree(5), Degree(6),
         Degree(6)});
    Configuration confs2_expected0 =
        Configuration(2, 4,
                      {Dart(0, 4, 1, -1), Dart(0, 7, -1, 0), Dart(1, 6, 3, -1), Dart(1, 8, 4, 2),
                       Dart(1, 0, -1, 3), Dart(2, 9, 6, -1), Dart(2, 2, -1, 5), Dart(3, 1, 8, -1),
                       Dart(3, 3, 9, 7), Dart(3, 5, -1, 8)},
                      {Degree(5), Degree(6), Degree(5), Degree(5)});
    Configuration confs2_expected1 =
        Configuration(2, 4,
                      {Dart(0, 4, -1, 1), Dart(0, 7, 0, -1), Dart(1, 6, -1, 3), Dart(1, 8, 2, 4),
                       Dart(1, 0, 3, -1), Dart(2, 9, -1, 6), Dart(2, 2, 5, -1), Dart(3, 1, -1, 8),
                       Dart(3, 3, 7, 9), Dart(3, 5, 8, -1)},
                      {Degree(5), Degree(6), Degree(5), Degree(5)});
    EXPECT_EQ(confs1.size(), 4);
    EXPECT_EQ(confs1[0], confs1_expected0);
    EXPECT_EQ(confs1[1], confs1_expected1);
    EXPECT_EQ(confs1[2], confs1_expected2);
    EXPECT_EQ(confs1[3], confs1_expected3);
    EXPECT_EQ(confs2.size(), 2);
    EXPECT_EQ(confs2[0], confs2_expected0);
    EXPECT_EQ(confs2[1], confs2_expected1);
}