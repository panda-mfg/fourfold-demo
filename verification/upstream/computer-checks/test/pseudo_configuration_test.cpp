#include "../src/pseudo_configuration.hpp"
#include "common.hpp"
#include <fmt/ranges.h>
#include <gtest/gtest.h>
#include <iostream>
#include <spdlog/spdlog.h>

TEST(PseudoConfigurationTest, Identify1) {
    int N                        = 3;
    vector<vector<int>> rotation = {{1, 2, -1}, {2, 0, -1}, {0, 1, -1}};
    vector<Degree> degrees       = {Degree(5, 6), Degree(6, 7), Degree(7)};
    PseudoConfiguration pc0      = PseudoConfiguration::from_v_rotations(N, rotation, degrees);
    PseudoConfiguration pc1      = PseudoConfiguration::from_v_rotations(N, rotation, degrees);
    vector<tuple<PseudoConfiguration, Mappings, Mappings>> pcs =
        PseudoConfiguration::free_homomorphism(pc0, pc1, 5,
                                               4); // identify (2,1) and (2,0)
    EXPECT_EQ(pcs.size(), 1);
    PseudoConfiguration pc_expected = PseudoConfiguration(
        4,
        {Dart(0, 2, 1, -1), Dart(0, 3, -1, 0), Dart(1, 0, -1, 5), Dart(3, 1, 8, -1),
         Dart(1, 7, 5, -1), Dart(1, 8, 2, 4), Dart(2, 9, 7, -1), Dart(2, 4, -1, 6),
         Dart(3, 5, 9, 3), Dart(3, 6, -1, 8)},
        {Degree(5, 6), Degree(6), Degree(6, 7), Degree(7)});
    vector<int> vmap0_expected = {0, 1, 3};
    vector<int> vmap1_expected = {1, 2, 3};
    vector<int> dmap0_expected = {0, 1, 5, 2, 3, 8};
    vector<int> dmap1_expected = {4, 5, 6, 7, 8, 9};
    EXPECT_EQ(std::get<0>(pcs[0]), pc_expected);
    EXPECT_EQ(std::get<1>(pcs[0]).vmap, vmap0_expected);
    EXPECT_EQ(std::get<1>(pcs[0]).dmap, dmap0_expected);
    EXPECT_EQ(std::get<2>(pcs[0]).vmap, vmap1_expected);
    EXPECT_EQ(std::get<2>(pcs[0]).dmap, dmap1_expected);
}

TEST(PseudoConfigurationTest, resolveDegreeIssues1) {
    int N                        = 7;
    vector<vector<int>> rotation = {
        {1, 2, 3, 4, 5, 6, -1}, {2, 0, -1},    {3, 0, 1, -1}, {4, 0, 2, -1},
        {5, 0, 3, -1},          {6, 0, 4, -1}, {0, 5, -1}};
    vector<Degree> degrees = {Degree(5, 8), Degree(6), Degree(6), Degree(6),
                              Degree(6),    Degree(6), Degree(6)};
    PseudoConfiguration pc = PseudoConfiguration::from_v_rotations(N, rotation, degrees);
    vector<pair<PseudoConfiguration, Mappings>> pcs = pc.resolve_degree_issues();
    EXPECT_EQ(pcs.size(), 3);
    PseudoConfiguration pc_expected0 = PseudoConfiguration(
        6, {Dart(0, 7, 1, 4),    Dart(0, 10, 2, 0),   Dart(0, 13, 3, 1),  Dart(0, 16, 4, 2),
            Dart(0, 18, 0, 3),   Dart(5, 8, 18, -1),  Dart(1, 11, 7, -1), Dart(1, 0, 8, 6),
            Dart(1, 5, -1, 7),   Dart(2, 14, 10, -1), Dart(2, 1, 11, 9),  Dart(2, 6, -1, 10),
            Dart(3, 17, 13, -1), Dart(3, 2, 14, 12),  Dart(3, 9, -1, 13), Dart(4, 19, 16, -1),
            Dart(4, 3, 17, 15),  Dart(4, 12, -1, 16), Dart(5, 4, 19, 5),  Dart(5, 15, -1, 18)},
        {Degree(5), Degree(6), Degree(6), Degree(6), Degree(6), Degree(6)});
    PseudoConfiguration pc_expected1 = PseudoConfiguration(
        7, {Dart(0, 7, 1, -1),   Dart(0, 9, 2, 0),    Dart(0, 12, 3, 1),   Dart(0, 15, 4, 2),
            Dart(0, 18, 5, 3),   Dart(0, 20, -1, 4),  Dart(1, 10, 7, -1),  Dart(1, 0, -1, 6),
            Dart(2, 13, 9, -1),  Dart(2, 1, 10, 8),   Dart(2, 6, -1, 9),   Dart(3, 16, 12, -1),
            Dart(3, 2, 13, 11),  Dart(3, 8, -1, 12),  Dart(4, 19, 15, -1), Dart(4, 3, 16, 14),
            Dart(4, 11, -1, 15), Dart(5, 21, 18, -1), Dart(5, 4, 19, 17),  Dart(5, 14, -1, 18),
            Dart(6, 5, 21, -1),  Dart(6, 17, -1, 20)},
        {Degree(7, 8), Degree(6), Degree(6), Degree(6), Degree(6), Degree(6), Degree(6)});
    PseudoConfiguration pc_expected2 = PseudoConfiguration(
        7, {Dart(0, 7, 1, 5),    Dart(0, 9, 2, 0),    Dart(0, 12, 3, 1),   Dart(0, 15, 4, 2),
            Dart(0, 18, 5, 3),   Dart(0, 20, 0, 4),   Dart(1, 10, 7, -1),  Dart(1, 0, 22, 6),
            Dart(2, 13, 9, -1),  Dart(2, 1, 10, 8),   Dart(2, 6, -1, 9),   Dart(3, 16, 12, -1),
            Dart(3, 2, 13, 11),  Dart(3, 8, -1, 12),  Dart(4, 19, 15, -1), Dart(4, 3, 16, 14),
            Dart(4, 11, -1, 15), Dart(5, 21, 18, -1), Dart(5, 4, 19, 17),  Dart(5, 14, -1, 18),
            Dart(6, 5, 21, 23),  Dart(6, 17, -1, 20), Dart(1, 23, -1, 7),  Dart(6, 22, 20, -1)},
        {Degree(6), Degree(6), Degree(6), Degree(6), Degree(6), Degree(6), Degree(6)});
    EXPECT_EQ(pcs[0].first, pc_expected0);
    EXPECT_EQ(pcs[1].first, pc_expected1);
    EXPECT_EQ(pcs[2].first, pc_expected2);
    vector<int> vmap_expected0 = {0, 5, 1, 2, 3, 4, 5};
    vector<int> dmap_expected0 = {4, 0,  1,  2,  3,  4,  5,  18, 6,  7,  8,
                                  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    vector<int> vmap_expected1 = {0, 1, 2, 3, 4, 5, 6};
    vector<int> dmap_expected1 = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};
    EXPECT_EQ(pcs[0].second.vmap, vmap_expected0);
    EXPECT_EQ(pcs[0].second.dmap, dmap_expected0);
    EXPECT_EQ(pcs[1].second.vmap, vmap_expected1);
    EXPECT_EQ(pcs[1].second.dmap, dmap_expected1);
    EXPECT_EQ(pcs[2].second.vmap, vmap_expected1);
    EXPECT_EQ(pcs[2].second.dmap, dmap_expected1);
}

TEST(PseudoConfigurationTest, resolveDegreeIssues2) {
    int N                        = 7;
    vector<vector<int>> rotation = {{1, 2, 3, 4, 5, 6}, {2, 0, 6, -1}, {3, 0, 1, -1}, {4, 0, 2, -1},
                                    {5, 0, 3, -1},      {6, 0, 4, -1}, {1, 0, 5, -1}};
    vector<Degree> degrees       = {Degree(5, 8), Degree(5), Degree(5), Degree(5),
                                    Degree(5),    Degree(5), Degree(5)};
    PseudoConfiguration pc       = PseudoConfiguration::from_v_rotations(N, rotation, degrees);
    vector<pair<PseudoConfiguration, Mappings>> pcs = pc.resolve_degree_issues();
    EXPECT_EQ(pcs.size(), 1);
    PseudoConfiguration pc_expected = PseudoConfiguration(
        7, {Dart(0, 7, 1, 5),    Dart(0, 10, 2, 0),   Dart(0, 13, 3, 1),   Dart(0, 16, 4, 2),
            Dart(0, 19, 5, 3),   Dart(0, 22, 0, 4),   Dart(1, 11, 7, -1),  Dart(1, 0, 8, 6),
            Dart(1, 21, -1, 7),  Dart(2, 14, 10, -1), Dart(2, 1, 11, 9),   Dart(2, 6, -1, 10),
            Dart(3, 17, 13, -1), Dart(3, 2, 14, 12),  Dart(3, 9, -1, 13),  Dart(4, 20, 16, -1),
            Dart(4, 3, 17, 15),  Dart(4, 12, -1, 16), Dart(5, 23, 19, -1), Dart(5, 4, 20, 18),
            Dart(5, 15, -1, 19), Dart(6, 8, 22, -1),  Dart(6, 5, 23, 21),  Dart(6, 18, -1, 22)},
        {Degree(6), Degree(5), Degree(5), Degree(5), Degree(5), Degree(5), Degree(5)});
    EXPECT_EQ(pcs[0].first, pc_expected);
    vector<int> vmap_expected = {0, 1, 2, 3, 4, 5, 6};
    vector<int> dmap_expected = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                                 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
    EXPECT_EQ(pcs[0].second.vmap, vmap_expected);
    EXPECT_EQ(pcs[0].second.dmap, dmap_expected);
}

TEST(PseudoConfigurationTest, resolveDegreeIssues3) {
    int N                         = 12;
    vector<vector<int>> rotations = {{1, 3, 8, 7, 2, -1}, {2, 5, 4, 3, 0, -1}, {0, 7, 6, 5, 1, -1},
                                     {0, 1, 4, 9, 8},     {1, 5, 10, 9, 3},    {1, 2, 6, 10, 4},
                                     {2, 7, 11, 10, 5},   {2, 0, 8, 11, 6},    {0, 3, 9, 11, 7},
                                     {3, 4, 10, 11, 8},   {4, 5, 6, 11, 9},    {6, 7, 8, 9, 10}};
    vector<Degree> degrees = {Degree(5), Degree(5), Degree(5), Degree(5), Degree(5), Degree(5),
                              Degree(5), Degree(5), Degree(5), Degree(5), Degree(5), Degree(5)};
    PseudoConfiguration icosahedral_graph =
        PseudoConfiguration::from_v_rotations(N, rotations, degrees);
    vector<pair<PseudoConfiguration, Mappings>> pcs = icosahedral_graph.resolve_degree_issues();
    EXPECT_EQ(pcs.size(), 1);
    PseudoConfiguration pc_expected = PseudoConfiguration(
        12, {Dart(0, 8, 1, 4),     Dart(0, 13, 2, 0),    Dart(0, 38, 3, 1),    Dart(0, 34, 4, 2),
             Dart(0, 9, 0, 3),     Dart(1, 23, 6, 58),   Dart(1, 18, 7, 5),    Dart(1, 14, 8, 6),
             Dart(1, 0, 58, 7),    Dart(2, 4, 10, 59),   Dart(2, 33, 11, 9),   Dart(2, 28, 12, 10),
             Dart(2, 24, 59, 11),  Dart(3, 1, 14, 17),   Dart(3, 7, 15, 13),   Dart(3, 22, 16, 14),
             Dart(3, 43, 17, 15),  Dart(3, 39, 13, 16),  Dart(4, 6, 19, 22),   Dart(4, 27, 20, 18),
             Dart(4, 48, 21, 19),  Dart(4, 44, 22, 20),  Dart(4, 15, 18, 21),  Dart(5, 5, 24, 27),
             Dart(5, 12, 25, 23),  Dart(5, 32, 26, 24),  Dart(5, 49, 27, 25),  Dart(5, 19, 23, 26),
             Dart(6, 11, 29, 32),  Dart(6, 37, 30, 28),  Dart(6, 53, 31, 29),  Dart(6, 50, 32, 30),
             Dart(6, 25, 28, 31),  Dart(7, 10, 34, 37),  Dart(7, 3, 35, 33),   Dart(7, 42, 36, 34),
             Dart(7, 54, 37, 35),  Dart(7, 29, 33, 36),  Dart(8, 2, 39, 42),   Dart(8, 17, 40, 38),
             Dart(8, 47, 41, 39),  Dart(8, 55, 42, 40),  Dart(8, 35, 38, 41),  Dart(9, 16, 44, 47),
             Dart(9, 21, 45, 43),  Dart(9, 52, 46, 44),  Dart(9, 56, 47, 45),  Dart(9, 40, 43, 46),
             Dart(10, 20, 49, 52), Dart(10, 26, 50, 48), Dart(10, 31, 51, 49), Dart(10, 57, 52, 50),
             Dart(10, 45, 48, 51), Dart(11, 30, 54, 57), Dart(11, 36, 55, 53), Dart(11, 41, 56, 54),
             Dart(11, 46, 57, 55), Dart(11, 51, 53, 56), Dart(1, 59, 5, 8),    Dart(2, 58, 9, 12)},
        {Degree(5), Degree(5), Degree(5), Degree(5), Degree(5), Degree(5), Degree(5), Degree(5),
         Degree(5), Degree(5), Degree(5), Degree(5)});
    EXPECT_EQ(pcs[0].first, pc_expected);
}

TEST(PseudoConfigurationTest, Identify2) {
    int N0                        = 6;
    vector<vector<int>> rotation0 = {{1, 2, 3, 4, 5}, {2, 0, 5, -1}, {3, 0, 1, -1},
                                     {4, 0, 2, -1},   {5, 0, 3, -1}, {1, 0, 4, -1}};
    vector<Degree> degrees0 = {Degree(5), Degree(5), Degree(5), Degree(5), Degree(6), Degree(5)};
    PseudoConfiguration pc0 = PseudoConfiguration::from_v_rotations(N0, rotation0, degrees0);

    int N1                        = 10;
    vector<vector<int>> rotation1 = {
        {1, 2, 9, -1}, {2, 0, -1},    {3, 9, 0, 1, -1}, {4, 7, 8, 9, 2, -1}, {5, 6, 7, 3, -1},
        {6, 4, -1},    {7, 4, 5, -1}, {8, 3, 4, 6, -1}, {9, 3, 7, -1},       {0, 2, 3, 8, -1}};
    vector<Degree> degrees1 = {Degree(6), Degree(5), Degree(5), Degree(6), Degree(5),
                               Degree(5), Degree(6), Degree(6), Degree(6), Degree(6)};
    PseudoConfiguration pc1 = PseudoConfiguration::from_v_rotations(N1, rotation1, degrees1);

    vector<tuple<PseudoConfiguration, Mappings, Mappings>> pcs =
        PseudoConfiguration::free_homomorphism(pc0, pc1, 14,
                                               13); // identify (4,5) and (3,2)
    EXPECT_EQ(pcs.size(), 1);
    PseudoConfiguration pc_expected0 = PseudoConfiguration(
        11, {Dart(0, 6, 1, 4),     Dart(0, 8, 2, 0),    Dart(0, 11, 3, 1),    Dart(0, 13, 4, 2),
             Dart(0, 15, 0, 3),    Dart(4, 9, 6, -1),   Dart(4, 0, 19, 5),    Dart(1, 12, 8, 31),
             Dart(1, 1, 9, 7),     Dart(1, 5, -1, 8),   Dart(2, 14, 11, 30),  Dart(2, 2, 12, 10),
             Dart(2, 7, 29, 11),   Dart(6, 3, 14, 28),  Dart(6, 10, 25, 13),  Dart(5, 4, 21, 24),
             Dart(3, 20, 17, -1),  Dart(3, 23, 18, 16), Dart(3, 42, -1, 17),  Dart(4, 24, 20, 6),
             Dart(4, 16, -1, 19),  Dart(5, 28, 22, 15), Dart(5, 43, 23, 21),  Dart(5, 17, 24, 22),
             Dart(5, 19, 15, 23),  Dart(6, 36, 26, 14), Dart(6, 40, 27, 25),  Dart(6, 44, 28, 26),
             Dart(6, 21, 13, 27),  Dart(2, 33, 30, 12), Dart(2, 37, 10, 29),  Dart(1, 34, 7, -1),
             Dart(7, 38, 33, -1),  Dart(7, 29, 34, 32), Dart(7, 31, -1, 33),  Dart(8, 41, 36, -1),
             Dart(8, 25, 37, 35),  Dart(8, 30, 38, 36), Dart(8, 32, -1, 37),  Dart(9, 45, 40, -1),
             Dart(9, 26, 41, 39),  Dart(9, 35, -1, 40), Dart(10, 18, 43, -1), Dart(10, 22, 44, 42),
             Dart(10, 27, 45, 43), Dart(10, 39, -1, 44)},
        {Degree(5), Degree(5), Degree(5), Degree(6), Degree(5), Degree(5), Degree(6), Degree(6),
         Degree(6), Degree(6), Degree(6)});
    EXPECT_EQ(std::get<0>(pcs[0]), pc_expected0);
    vector<int> vmap0_expected = {0, 4, 1, 2, 6, 5};
    vector<int> vmap1_expected = {3, 4, 5, 6, 2, 1, 7, 8, 9, 10};
    vector<int> dmap0_expected = {0, 1,  2,  3,  4,  5,  6,  19, 7,  8,
                                  9, 10, 11, 12, 28, 13, 14, 24, 15, 21};
    vector<int> dmap1_expected = {16, 17, 18, 19, 20, 21, 22, 23, 24, 14, 25, 26,
                                  27, 28, 12, 29, 30, 10, 31, 7,  32, 33, 34, 35,
                                  36, 37, 38, 39, 40, 41, 42, 43, 44, 45};
    EXPECT_EQ(std::get<1>(pcs[0]).vmap, vmap0_expected);
    EXPECT_EQ(std::get<2>(pcs[0]).vmap, vmap1_expected);
    EXPECT_EQ(std::get<1>(pcs[0]).dmap, dmap0_expected);
    EXPECT_EQ(std::get<2>(pcs[0]).dmap, dmap1_expected);
}

TEST_F(ConfFiles, Contain1) {
    CartWheel cw                 = CartWheel::generate_cartwheel(7, {6, 6, 6, 6, 6, 6, 6});
    vector<Configuration> confs1 = Configuration::from_file(conffile1);
    cw.degrees[9]                = Degree(5);
    cw.degrees[10]               = Degree(5);
    cw.degrees[12]               = Degree(5);
    cw.degrees[13]               = Degree(5);
    EXPECT_TRUE(cw.blocked_by_reducible_configuration(cw.center, confs1));
}

TEST_F(ConfFiles, Contain2) {
    CartWheel cw                 = CartWheel::generate_cartwheel(7, {5, 6, 6, 6, 6, 6, 5});
    vector<Configuration> confs2 = Configuration::from_file(conffile2);
    cw.degrees[8]                = Degree(6);
    EXPECT_FALSE(cw.blocked_by_reducible_configuration(cw.center, confs2));
    cw.degrees[9] = Degree(5);
    EXPECT_TRUE(cw.blocked_by_reducible_configuration(cw.center, confs2));
}

TEST_F(RuleFiles, AmountChargeSend) {
    Rule rule1         = Rule::from_file(rulefile1);
    Rule rule2         = Rule::from_file(rulefile2);
    Rule rule3         = Rule::from_file(rulefile3);
    vector<Rule> rules = {rule1, rule2, rule3};

    CartWheel cw   = CartWheel::generate_cartwheel(7, {5, 5, 5, 5, 7, 5, 7});
    cw.degrees[8]  = Degree(6);
    cw.degrees[18] = Degree(5);

    EXPECT_EQ(cw.amount_of_charge_send(28, rules), 1); // (5,0)
    EXPECT_EQ(cw.amount_of_charge_send(41, rules), 0); // (7,0)
    EXPECT_EQ(cw.amount_of_charge_send(23, rules), 0); // (4,0)
    EXPECT_EQ(cw.amount_of_charge_send(6, rules), 1);  // (0,7)
    EXPECT_EQ(cw.amount_of_charge_send(0, rules), 2);  // (0,1)
}

TEST_F(RuleFiles, AmountPossibleChargeSend) {
    Rule rule1                          = Rule::from_file(rulefile1);
    Rule rule2                          = Rule::from_file(rulefile2);
    Rule rule3                          = Rule::from_file(rulefile3);
    vector<Rule> rules                  = {rule1, rule2, rule3};
    vector<CombinedRule> combined_rules = combine_rules(rules, {});

    CartWheel cw   = CartWheel::generate_cartwheel(8, {5, 7, 5, 7, 5, 9, 9, 9});
    cw.degrees[13] = Degree(6);
    cw.degrees[14] = Degree(7);

    EXPECT_EQ(cw.amount_of_possible_charge_send(1, combined_rules), 1); // (head,tail) = (0,2)
    EXPECT_EQ(cw.amount_of_possible_charge_send(2, combined_rules), 2); // (0,3)
    EXPECT_EQ(cw.amount_of_possible_charge_send(3, combined_rules), 2); // (0,4)
}

TEST(PseudoConfigurationTest, findHomomorphism) {
    PseudoConfiguration pc0 = PseudoConfiguration::from_v_rotations(
        5, {{1, 2, 3, 4, -1}, {2, 0, -1}, {3, 0, 1, -1}, {4, 0, 2, -1}, {0, 3, -1}},
        {Degree(6), Degree(5), Degree(6), Degree(6), Degree(5)});
    PseudoConfiguration pc1 = PseudoConfiguration::from_v_rotations(
        7,
        {{1, 2, 3, 4, 5, 6},
         {2, 0, 6, -1},
         {3, 0, 1, -1},
         {4, 0, 2, -1},
         {5, 0, 3, -1},
         {6, 0, 4, -1},
         {1, 0, 5, -1}},
        {Degree(6), Degree(5, INFTY), Degree(6), Degree(6), Degree(5), Degree(6), Degree(6)});
    EXPECT_TRUE(PseudoConfiguration::homomorphism(pc0, 0, pc1, 0, Degree::has_intersection)
                    .has_value()); // (0,1) -> (0,1)
    EXPECT_FALSE(PseudoConfiguration::homomorphism(pc0, 0, pc1, 8, Degree::has_intersection)
                     .has_value()); // (0,1) -> (6,1)
}
