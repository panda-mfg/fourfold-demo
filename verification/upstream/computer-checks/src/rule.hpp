#ifndef RULE_HPP
#define RULE_HPP
#include "configuration.hpp"
#include "pseudo_configuration.hpp"

struct Rule : public PseudoConfiguration {
    int st_id;
    int amount;
    Rule(int st_id = -1, int amount = 0, int N = 0, const vector<Dart> &darts = vector<Dart>(),
         const vector<Degree> &degrees = vector<Degree>())
        : PseudoConfiguration(N, darts, degrees), st_id(st_id), amount(amount) {}
    bool operator==(const Rule &other) const = default;
    string to_string(void) const;
    static Rule read(std::ifstream &ifs);
    void write(std::ofstream &ofs) const;
    static Rule from_file(const string &filename);
    void to_file(const string &filename) const;
    static vector<Rule> get_rules(const string &ruledir);
};

struct CombinedRule : public Rule {
    vector<bool> combined_flag;
    CombinedRule(const vector<bool> &combined_flag = vector<bool>(), int st_id = -1, int amount = 0,
                 int N = 0, const vector<Dart> &darts = vector<Dart>(),
                 const vector<Degree> &degrees = vector<Degree>())
        : Rule(st_id, amount, N, darts, degrees), combined_flag(combined_flag) {}
    bool operator==(const CombinedRule &other) const = default;
    static CombinedRule from_file(const string &filename);
    void to_file(const string &filename) const;
    static vector<CombinedRule> get_combined_rules(const string &combined_ruledir);
    vector<CombinedRule> add_rule_to_combination(const vector<Rule> &rules, size_t i,
                                                 const vector<Configuration> &confs) const;
};

vector<CombinedRule> combine_rules(const vector<Rule> &rules, const vector<Configuration> &confs);
void run_combine_rules(const string &confdir, const string &ruledir, const string &outdir);

#endif // RULE_HPP