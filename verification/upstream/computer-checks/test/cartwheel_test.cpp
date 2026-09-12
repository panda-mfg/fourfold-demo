#include "common.hpp"

TEST_F(CartWheelFiles, FromFile) {
    CartWheel cw1 = CartWheel::from_file(cartwheelfile1);
    CartWheel cw2 = CartWheel::from_file(cartwheelfile2);
    CartWheel expected_cw1(
        0, {0, 1, 2, 3, 4, 5, 6}, 8,
        {Dart(0, 8, 1, 6),    Dart(0, 11, 2, 0),   Dart(0, 14, 3, 1),   Dart(0, 17, 4, 2),
         Dart(0, 20, 5, 3),   Dart(0, 23, 6, 4),   Dart(0, 26, 0, 5),   Dart(1, 12, 8, -1),
         Dart(1, 0, 9, 7),    Dart(1, 25, -1, 8),  Dart(2, 15, 11, -1), Dart(2, 1, 12, 10),
         Dart(2, 7, -1, 11),  Dart(3, 18, 14, -1), Dart(3, 2, 15, 13),  Dart(3, 10, -1, 14),
         Dart(4, 21, 17, -1), Dart(4, 3, 18, 16),  Dart(4, 13, -1, 17), Dart(5, 24, 20, -1),
         Dart(5, 4, 21, 19),  Dart(5, 16, -1, 20), Dart(6, 27, 23, -1), Dart(6, 5, 24, 22),
         Dart(6, 19, -1, 23), Dart(7, 9, 26, -1),  Dart(7, 6, 27, 25),  Dart(7, 22, -1, 26)},
        {Degree(7), Degree(5), Degree(5), Degree(6), Degree(5), Degree(5), Degree(5), Degree(9)});
    CartWheel expected_cw2(
        0, {0, 1, 2, 3, 4, 5, 6}, 18,
        {Dart(0, 7, 1, 6),     Dart(0, 12, 2, 0),    Dart(0, 19, 3, 1),    Dart(0, 24, 4, 2),
         Dart(0, 31, 5, 3),    Dart(0, 34, 6, 4),    Dart(0, 40, 0, 5),    Dart(1, 0, 8, 11),
         Dart(1, 39, 9, 7),    Dart(1, 46, 10, 8),   Dart(1, 51, 11, 9),   Dart(1, 13, 7, 10),
         Dart(2, 1, 13, 18),   Dart(2, 11, 14, 12),  Dart(2, 50, 15, 13),  Dart(2, 54, 16, 14),
         Dart(2, 57, 17, 15),  Dart(2, 61, 18, 16),  Dart(2, 20, 12, 17),  Dart(3, 2, 20, 23),
         Dart(3, 18, 21, 19),  Dart(3, 60, 22, 20),  Dart(3, 65, 23, 21),  Dart(3, 25, 19, 22),
         Dart(4, 3, 25, 28),   Dart(4, 23, 26, 24),  Dart(4, 64, 27, 25),  Dart(4, 68, 28, 26),
         Dart(4, 32, 24, 27),  Dart(5, 72, 30, -1),  Dart(5, 35, 31, 29),  Dart(5, 4, 32, 30),
         Dart(5, 28, 33, 31),  Dart(5, 67, -1, 32),  Dart(6, 5, 35, 38),   Dart(6, 30, 36, 34),
         Dart(6, 71, 37, 35),  Dart(6, 75, 38, 36),  Dart(6, 41, 34, 37),  Dart(7, 8, 40, 44),
         Dart(7, 6, 41, 39),   Dart(7, 38, 42, 40),  Dart(7, 74, 43, 41),  Dart(7, 78, 44, 42),
         Dart(7, 47, 39, 43),  Dart(8, 52, 46, -1),  Dart(8, 9, 47, 45),   Dart(8, 44, 48, 46),
         Dart(8, 77, -1, 47),  Dart(9, 55, 50, -1),  Dart(9, 14, 51, 49),  Dart(9, 10, 52, 50),
         Dart(9, 45, -1, 51),  Dart(10, 58, 54, -1), Dart(10, 15, 55, 53), Dart(10, 49, -1, 54),
         Dart(11, 62, 57, -1), Dart(11, 16, 58, 56), Dart(11, 53, -1, 57), Dart(12, 66, 60, -1),
         Dart(12, 21, 61, 59), Dart(12, 17, 62, 60), Dart(12, 56, -1, 61), Dart(13, 69, 64, -1),
         Dart(13, 26, 65, 63), Dart(13, 22, 66, 64), Dart(13, 59, -1, 65), Dart(14, 33, 68, -1),
         Dart(14, 27, 69, 67), Dart(14, 63, -1, 68), Dart(15, 76, 71, -1), Dart(15, 36, 72, 70),
         Dart(15, 29, -1, 71), Dart(16, 79, 74, -1), Dart(16, 42, 75, 73), Dart(16, 37, 76, 74),
         Dart(16, 70, -1, 75), Dart(17, 48, 78, -1), Dart(17, 43, 79, 77), Dart(17, 73, -1, 78)},
        {Degree(7), Degree(5), Degree(7), Degree(5), Degree(5), Degree(9), Degree(5), Degree(6),
         Degree(5, 9), Degree(5, 9), Degree(5, 9), Degree(5, 9), Degree(5, 9), Degree(5, 9),
         Degree(5, 9), Degree(5, 9), Degree(5, 9), Degree(5, 9)});
    EXPECT_EQ(cw1, expected_cw1);
    EXPECT_EQ(cw2, expected_cw2);
}

TEST(CartWheelTest, EnumWheels) {
    // 1/d * \sum_{i=1}^d 5^{gcd(i,d)} by Burnside's lemma
    vector<CartWheel> wheel5s = CartWheel::enum_wheels(5);
    EXPECT_EQ(wheel5s.size(), 629);
    vector<CartWheel> wheel6s = CartWheel::enum_wheels(6);
    EXPECT_EQ(wheel6s.size(), 2635);
    vector<CartWheel> wheel7s = CartWheel::enum_wheels(7);
    EXPECT_EQ(wheel7s.size(), 11165);
}

TEST_F(RuleFiles, Charge) {
    vector<Rule> rules                  = {Rule::from_file(rulefile1), Rule::from_file(rulefile2),
                                           Rule::from_file(rulefile3)};
    vector<CombinedRule> combined_rules = combine_rules(rules, {});

    vector<CartWheel> wheels = {CartWheel::generate_cartwheel(7, {5, 7, 5, 5, 9, 5, 6}),
                                CartWheel::generate_cartwheel(7, {5, 5, 7, 5, 7, 5, 7})};

    vector<pair<int, int>> expected_values = {
        {1, 8},
        {0, 8},
    };
    for (size_t i = 0; i < wheels.size(); i++) {
        int out_charge = 0;
        int in_charge  = 0;
        for (size_t j = 0; j < wheels[i].center_darts.size(); j++) {
            int dart_id = wheels[i].center_darts[j];
            in_charge += wheels[i].amount_of_charge_send(dart_id, rules);
            int rev_dart_id = wheels[i].darts[dart_id].rev;
            out_charge += wheels[i].amount_of_charge_send(rev_dart_id, rules);
        }
        EXPECT_EQ(expected_values[i].first, out_charge);
        EXPECT_EQ(expected_values[i].second, in_charge);
    }
}

TEST_F(RuleFiles, PruneByCharge) {
    vector<Rule> rules                  = {Rule::from_file(rulefile1), Rule::from_file(rulefile2),
                                           Rule::from_file(rulefile3)};
    vector<CombinedRule> combined_rules = combine_rules(rules, {});

    CartWheel wheel = CartWheel::generate_cartwheel(7, {7, 5, 7, 5, 7, 5, 5});
    vector<CombinedRule> combined_rule_with_spokes1 = {combined_rules[0], combined_rules[1]};
    EXPECT_FALSE(wheel.prune_by_non_associated_rule(combined_rule_with_spokes1, rules));
    EXPECT_GT(wheel.upper_bound_of_charge(combined_rule_with_spokes1, rules, combined_rules), 0);

    wheel.degrees[10]                               = Degree(5);
    wheel.degrees[11]                               = Degree(6);
    wheel.degrees[12]                               = Degree(5);
    vector<CombinedRule> combined_rule_with_spokes2 = {combined_rules[4], combined_rules[1]};
    EXPECT_FALSE(wheel.prune_by_non_associated_rule(combined_rule_with_spokes2, rules));
    EXPECT_GT(wheel.upper_bound_of_charge(combined_rule_with_spokes2, rules, combined_rules), 0);
    vector<CombinedRule> combined_rule_with_spokes3 = {combined_rules[4], combined_rules[1],
                                                       combined_rules[0], combined_rules[1]};
    EXPECT_FALSE(wheel.prune_by_non_associated_rule(combined_rule_with_spokes3, rules));
    EXPECT_GT(wheel.upper_bound_of_charge(combined_rule_with_spokes3, rules, combined_rules), 0);
}

TEST_F(RuleFiles, refinement1) {
    CartWheel cw       = CartWheel::generate_cartwheel(7, {7, 7, 5, 5, 9, 6, 5});
    cw.degrees[14]     = Degree(6);
    Rule rule4         = Rule::from_file(rulefile4);
    bool should_refine = cw.should_refine(1, rule4);
    ASSERT_TRUE(should_refine);
    vector<CartWheel> refinements = cw.refinement(1, rule4);
    EXPECT_EQ(refinements.size(), 4);
    vector<CartWheel> expected;
    expected.push_back(cw);
    expected.back().degrees[11] = Degree(5);
    expected.push_back(cw);
    expected.back().degrees[15] = Degree(5);
    expected.push_back(cw);
    expected.back().degrees[15] = Degree(6);
    expected.push_back(cw);
    expected.back().degrees[11] = Degree(6, 9);
    expected.back().degrees[15] = Degree(7, 9);
    EXPECT_EQ(refinements, expected);
}

TEST_F(RuleFiles, refinement2) {
    CartWheel cw       = CartWheel::generate_cartwheel(7, {7, 7, 5, 5, 9, 6, 5});
    Rule rule4         = Rule::from_file(rulefile4);
    bool should_refine = cw.should_refine(1, rule4);
    EXPECT_FALSE(should_refine);
}

TEST_F(RuleFiles, enumBadCartWheels1) {
    vector<Rule> rules                  = {Rule::from_file(rulefile1), Rule::from_file(rulefile2),
                                           Rule::from_file(rulefile3), Rule::from_file(rulefile4)};
    vector<CombinedRule> combined_rules = combine_rules(rules, {});
    CartWheel cw                        = CartWheel::generate_cartwheel(7, {5, 7, 5, 7, 5, 8, 9});
    vector<CartWheel> enumerated        = cw.enum_bad_cartwheels(rules, combined_rules, {});
    CartWheel expected                  = cw;
    expected.degrees[11]                = Degree(5);
    expected.degrees[12]                = Degree(6);
    expected.degrees[13]                = Degree(5);
    expected.degrees[15]                = Degree(5);
    expected.degrees[16]                = Degree(6);
    expected.degrees[17]                = Degree(5);
    EXPECT_EQ(enumerated.size(), 1);
    EXPECT_EQ(enumerated[0], expected);
}

TEST_F(RuleFiles, enumBadCartWheels2) {
    vector<Rule> rules                  = {Rule::from_file(rulefile1), Rule::from_file(rulefile2),
                                           Rule::from_file(rulefile3), Rule::from_file(rulefile4)};
    vector<CombinedRule> combined_rules = combine_rules(rules, {});
    CartWheel cw                        = CartWheel::generate_cartwheel(7, {5, 5, 5, 7, 7, 5, 7});
    vector<CartWheel> enumerated1       = cw.enum_bad_cartwheels(rules, combined_rules, {});
    CartWheel expected1                 = cw;
    expected1.degrees[8]                = Degree(6);
    expected1.degrees[9]                = Degree(5);
    expected1.degrees[20]               = Degree(5);
    EXPECT_EQ(enumerated1.size(), 1);
    EXPECT_EQ(enumerated1[0], expected1);
    cw.degrees[17]                = Degree(6);
    vector<CartWheel> enumerated2 = cw.enum_bad_cartwheels(rules, combined_rules, {});
    expected1.degrees[17]         = Degree(6);
    vector<CartWheel> expected2   = {expected1, expected1, expected1};
    expected2[0].degrees[14]      = Degree(5);
    expected2[1].degrees[18]      = Degree(5);
    expected2[2].degrees[18]      = Degree(6);
    EXPECT_EQ(enumerated2, expected2);
}
