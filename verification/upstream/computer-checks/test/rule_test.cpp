#include "common.hpp"

TEST_F(RuleFiles, ReadRuleFile) {
    Rule rule1 = Rule::from_file(rulefile1);
    Rule rule2 = Rule::from_file(rulefile2);
    Rule rule1_expected =
        Rule(1, 2, 2, {Dart(0, 1, -1, -1), Dart(1, 0, -1, -1)}, {Degree(5), Degree(5, 1000000000)});
    Rule rule2_expected =
        Rule(5, 1, 6,
             {Dart(0, 15, 1, -1), Dart(0, 12, 2, 0), Dart(0, 9, 3, 1), Dart(0, 5, 4, 2),
              Dart(0, 16, -1, 3), Dart(1, 3, 6, 7), Dart(1, 8, -1, 5), Dart(1, 17, 5, -1),
              Dart(2, 6, 9, -1), Dart(2, 2, 10, 8), Dart(2, 11, -1, 9), Dart(3, 10, 12, -1),
              Dart(3, 1, 13, 11), Dart(3, 14, -1, 12), Dart(4, 13, 15, -1), Dart(4, 0, -1, 14),
              Dart(5, 4, 17, -1), Dart(5, 7, -1, 16)},
             {Degree(7), Degree(7, 1000000000), Degree(5), Degree(5, 6), Degree(5), Degree(5)});
    EXPECT_EQ(rule1, rule1_expected);
    EXPECT_EQ(rule2, rule2_expected);
}

TEST_F(RuleFiles, CombineRules) {
    Rule rule1         = Rule::from_file(rulefile1);
    Rule rule2         = Rule::from_file(rulefile2);
    Rule rule3         = Rule::from_file(rulefile3);
    vector<Rule> rules = {rule1, rule2, rule3};

    vector<CombinedRule> combined_rules          = combine_rules(rules, {});
    vector<CombinedRule> combined_rules_expected = {
        CombinedRule({false, false, false}, 0, 0, 2, {Dart(0, 1, -1, -1), Dart(1, 0, -1, -1)},
                     {Degree(1, INFTY), Degree(1, INFTY)}),
        CombinedRule({true, false, false}, 1, 2, 2, {Dart(0, 1, -1, -1), Dart(1, 0, -1, -1)},
                     {Degree(5), Degree(5, INFTY)}),
        CombinedRule({false, true, false}, 5, 1, 6,
                     {Dart(0, 15, 1, -1), Dart(0, 12, 2, 0), Dart(0, 9, 3, 1), Dart(0, 5, 4, 2),
                      Dart(0, 16, -1, 3), Dart(1, 3, 6, 7), Dart(1, 8, -1, 5), Dart(1, 17, 5, -1),
                      Dart(2, 6, 9, -1), Dart(2, 2, 10, 8), Dart(2, 11, -1, 9), Dart(3, 10, 12, -1),
                      Dart(3, 1, 13, 11), Dart(3, 14, -1, 12), Dart(4, 13, 15, -1),
                      Dart(4, 0, -1, 14), Dart(5, 4, 17, -1), Dart(5, 7, -1, 16)},
                     {Degree(7), Degree(7, INFTY), Degree(5), Degree(5, 6), Degree(5), Degree(5)}),
        CombinedRule({false, false, true}, 5, 1, 6,
                     {Dart(0, 11, 1, -1), Dart(0, 15, 2, 0), Dart(0, 5, 3, 1), Dart(0, 7, -1, 2),
                      Dart(1, 8, 5, -1), Dart(1, 2, 6, 4), Dart(1, 14, -1, 5), Dart(2, 3, 8, -1),
                      Dart(2, 4, -1, 7), Dart(3, 13, 10, -1), Dart(3, 16, 11, 9),
                      Dart(3, 0, -1, 10), Dart(4, 17, 13, -1), Dart(4, 9, -1, 12),
                      Dart(5, 6, 15, -1), Dart(5, 1, 16, 14), Dart(5, 10, 17, 15),
                      Dart(5, 12, -1, 16)},
                     {Degree(7), Degree(7, INFTY), Degree(5), Degree(6), Degree(5), Degree(5)}),
        CombinedRule(
            {false, true, true}, 9, 2, 7,
            {Dart(1, 3, 4, -1),   Dart(4, 2, -1, 15),  Dart(0, 1, 3, -1),   Dart(0, 0, -1, 2),
             Dart(1, 15, 5, 0),   Dart(1, 19, 6, 4),   Dart(1, 9, 7, 5),    Dart(1, 11, -1, 6),
             Dart(2, 12, 9, -1),  Dart(2, 6, 10, 8),   Dart(2, 18, -1, 9),  Dart(3, 7, 12, -1),
             Dart(3, 8, -1, 11),  Dart(4, 17, 14, -1), Dart(4, 20, 15, 13), Dart(4, 4, 1, 14),
             Dart(5, 21, 17, -1), Dart(5, 13, -1, 16), Dart(6, 10, 19, -1), Dart(6, 5, 20, 18),
             Dart(6, 14, 21, 19), Dart(6, 16, -1, 20)},
            {Degree(5), Degree(7), Degree(7, INFTY), Degree(5), Degree(6), Degree(5), Degree(5)})};
    EXPECT_EQ(combined_rules.size(), 5);
    for (size_t i = 0; i < combined_rules.size(); i++) {
        EXPECT_EQ(combined_rules[i], combined_rules_expected[i]);
    }
}
