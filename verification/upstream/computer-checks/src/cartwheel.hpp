#ifndef CARTWHEEL_HPP
#define CARTWHEEL_HPP
#include "pseudo_configuration.hpp"
#include <array>
using std::array;

struct CartWheel : public PseudoConfiguration {
    int center;
    vector<int> center_darts;
    CartWheel(int center, const vector<int> &center_darts, int N, const vector<Dart> &darts,
              const vector<Degree> &degrees)
        : PseudoConfiguration(N, darts, degrees), center(center), center_darts(center_darts) {}
    bool operator==(const CartWheel &other) const = default;
    string to_string(void) const;
    static CartWheel from_file(const string &filename);
    void to_file(const string &filename) const;
    static vector<CartWheel> get_cartwheels(const string &cartwheeldir);

    // Enumerating degrees of neighbors
    static vector<CartWheel> enum_wheels(int center_degree);
    static CartWheel generate_cartwheel(int deg, const vector<int> &degrees);
    static vector<CartWheel> enum_possible_bad_wheels(int center_degree, const vector<Rule> &rules,
                                                      const vector<CombinedRule> &combined_rules,
                                                      const vector<Configuration> &confs);

    // Fixing rules applied from neighbors to the center
    vector<pair<CartWheel, vector<CombinedRule>>>
    fix_in_rules(const vector<Rule> &rules, const vector<CombinedRule> &combined_rules,
                 const vector<Configuration> &confs) const;
    vector<CartWheel> update_degree_by_rule(int dart_id, const Rule &rule) const;
    vector<CartWheel> concrete_degree_except_tail(void) const;

    // Pruning
    bool prune(const vector<CombinedRule> &combined_rule_with_spokes, const vector<Rule> &rules,
               const vector<CombinedRule> &combined_rules,
               const vector<Configuration> &confs) const;
    bool prune_by_non_associated_rule(const vector<CombinedRule> &combined_rule_with_spokes,
                                      const vector<Rule> &rules) const;
    int upper_bound_of_charge(const vector<CombinedRule> &combined_rule_with_spokes,
                              const vector<Rule> &rules,
                              const vector<CombinedRule> &combined_rules) const;

    // Fixing rules applied from the center to neighbors
    vector<pair<CartWheel, vector<CombinedRule>>>
    fix_out_rules(const vector<pair<CartWheel, vector<CombinedRule>>> &cartwheels,
                  const vector<Rule> &rules, const vector<CombinedRule> &combined_rules,
                  const vector<Configuration> &confs) const;
    // Refinement
    bool should_refine(int i, const Rule &rule) const;
    vector<CartWheel> refinement(int i, const Rule &rule) const;
    CartWheel refine_always(const vector<int> &vs_refine, const Mappings &rule2cw,
                            const Rule &rule) const;
    vector<CartWheel> refine_never(const vector<int> &vs_refine, const Mappings &rule2cw,
                                   const Rule &rule) const;

    // Overall algorithm
    vector<CartWheel> enum_bad_cartwheels(const vector<Rule> &rules,
                                          const vector<CombinedRule> &combined_rules,
                                          const vector<Configuration> &confs) const;
    array<vector<int>, CARTWHEEL_DEG_MAX + 1> center_darts_by_degree(void) const;
};

void run_enum_wheels(int center_degree, const string &confdir, const string &ruledir,
                     const string &combined_ruledir, const string &outdir);
void run_enum_cartwheels(const string &wheel_file, const string &confdir, const string &ruledir,
                         const string &combined_ruledir, const string &outdir);

#endif // CARTWHEEL_HPP