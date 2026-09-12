#ifndef PSEUDO_CONFIGURATION_HPP
#define PSEUDO_CONFIGURATION_HPP
#include "degree.hpp"
#include "pseudo_triangulation.hpp"
#include "util.hpp"
#include <numeric>
#include <optional>
#include <variant>
using std::optional;

struct Rule;
struct CombinedRule;
struct Configuration;
struct CartWheel;

// DegreeTest is a function that takes two Degree objects and returns a type convertible to bool
template <typename Func>
concept DegreeTest = std::predicate<Func, Degree, Degree>;

struct PseudoConfiguration : public PseudoTriangulation {
    vector<Degree> degrees;

    PseudoConfiguration(int N = 0, const vector<Dart> &darts = vector<Dart>(),
                        const vector<Degree> &degrees = vector<Degree>())
        : PseudoTriangulation(N, darts), degrees(degrees) {}
    bool operator==(const PseudoConfiguration &other) const = default;
    string debug(void) const;
    string to_string(void) const;
    static PseudoConfiguration from_v_rotations(int N, const vector<vector<int>> &v_rotations,
                                                const vector<Degree> &degrees);
    static PseudoConfiguration disjoint_union(const PseudoConfiguration &L,
                                              const PseudoConfiguration &R);

    // homomorphism
    template <typename DegreeTest>
    static optional<Mappings> homomorphism(const PseudoConfiguration &from, int dart_from,
                                           const PseudoConfiguration &to, int dart_to,
                                           DegreeTest degree_test);

    // free homomorphism
    static vector<tuple<PseudoConfiguration, Mappings, Mappings>>
    free_homomorphism(const PseudoConfiguration &pc0, const PseudoConfiguration &pc1, int dart_id0,
                      int dart_id1);
    optional<pair<PseudoConfiguration, Mappings>>
    dart_identification(const vector<pair<int, int>> &dart_pairs) const;
    vector<pair<PseudoConfiguration, Mappings>>
    free_homomorphism(const vector<pair<int, int>> &dart_pairs) const;

    // resolving degree issues
    vector<pair<PseudoConfiguration, Mappings>> resolve_degree_issues(void) const;
    bool inner_subdegree_error(void) const;
    optional<int> vertex_single_degree_issue(void) const;
    optional<pair<PseudoConfiguration, Mappings>> fix_single_degree_issue(int v) const;
    optional<PseudoConfiguration> add_boundary_darts(int v) const;
    optional<pair<PseudoConfiguration, PseudoConfiguration>> single_out_lower_degree(void) const;

    // reducible configurations in pseudo-configurations
    bool contain_conf(int center, const vector<Configuration> &confs) const;
    vector<vector<vector<int>>> darts_by_degree(void) const;
    bool rooted_contain_conf(int dart_id, const Configuration &conf) const;

    // blocked by reducible configurations
    bool blocked_by_reducible_configuration(int center, const vector<Configuration> &confs) const;
    vector<PseudoConfiguration> representative_degree(int center) const;

    // charges along an edge and bound on the final charge
    bool always_apply(int dart_id, const Rule &rule) const;
    bool never_apply(int dart_id, const Rule &rule) const;
    int amount_of_charge_send(int dart_id, const vector<Rule> &rules) const;
    int amount_of_possible_charge_send(int dart_id,
                                       const vector<CombinedRule> &combined_rules) const;

    // refinement
    bool dominantly_apply(int dart_id, const Rule &rule) const;

    // Free combinations of free cartwheels
    vector<pair<PseudoConfiguration, Mappings>>
    combine_each_cartwheel(int dart_id, const vector<CartWheel> &cartwheels,
                           const vector<Configuration> &confs) const;
    vector<pair<PseudoConfiguration, Mappings>>
    combine_each_cartwheel_twice(int dart_id1, int dart_id2, const vector<CartWheel> &cartwheels,
                                 const vector<Configuration> &confs) const;
};

template <typename DegreeTest>
optional<Mappings> PseudoConfiguration::homomorphism(const PseudoConfiguration &Z, int e,
                                                     const PseudoConfiguration &Z_star, int e_star,
                                                     DegreeTest degree_test) {
    vector<int> vmap(Z.N, -1);
    vector<int> dmap(Z.darts.size(), -1);
    queue<pair<int, int>> Q;
    Q.emplace(e, e_star);
    while (!Q.empty()) {
        auto [f, f_star] = Q.front();
        Q.pop();
        if (dmap[f] != -1) {
            if (dmap[f] != f_star) {
                return std::nullopt;
            }
            continue;
        }
        dmap[f]    = f_star;
        int h      = Z.darts[f].head;
        int h_star = Z_star.darts[f_star].head;
        if (vmap[h] != -1 && vmap[h] != h_star) {
            return std::nullopt;
        }
        vmap[h] = h_star;
        if (!degree_test(Z.degrees[h], Z_star.degrees[h_star])) {
            return std::nullopt;
        }
        int rev      = Z.darts[f].rev;
        int rev_star = Z_star.darts[f_star].rev;
        Q.push({rev, rev_star});
        int succ      = Z.darts[f].succ;
        int succ_star = Z_star.darts[f_star].succ;
        if (succ != nil && succ_star == nil) {
            return std::nullopt;
        } else if (succ != nil && succ_star != nil) {
            Q.push({succ, succ_star});
        }
        int pred      = Z.darts[f].pred;
        int pred_star = Z_star.darts[f_star].pred;
        if (pred != nil && pred_star == nil) {
            return std::nullopt;
        } else if (pred != nil && pred_star != nil) {
            Q.push({pred, pred_star});
        }
    }
    return Mappings(vmap, dmap);
}

#endif // PSEUDO_CONFIGURATION_HPP
