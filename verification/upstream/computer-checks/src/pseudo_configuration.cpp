#include "pseudo_configuration.hpp"
#include "cartwheel.hpp"
#include "configuration.hpp"
#include "rule.hpp"
#include <cassert>
#include <fmt/core.h>
#include <queue>
using std::queue;

string PseudoConfiguration::debug(void) const {
    string res = PseudoTriangulation::debug();
    for (int v = 0; v < N; v++) {
        res += fmt::format("Degree({}, {}),\n", degrees[v].lower, degrees[v].upper);
    }
    return res;
}

string PseudoConfiguration::to_string(void) const {
    string res = fmt::format("N: {}\n", N);
    vector<pair<int, int>> edges(darts.size());
    for (size_t i = 0; i < darts.size(); i++) {
        edges[i] = {darts[i].head, darts[darts[i].rev].head};
    }
    vector<vector<int>> e_rotations = this->get_e_rotations();
    for (int v = 0; v < N; v++) {
        res += fmt::format("{}, deg=({}, {}): ", v, degrees[v].lower, degrees[v].upper);
        for (size_t i = 0; i < e_rotations[v].size(); i++) {
            int dart_id = e_rotations[v][i];
            if (dart_id == -1) {
                res += "nil, ";
            } else {
                res += fmt::format("e{}({}-{}), ", dart_id, edges[dart_id].first,
                                   edges[dart_id].second);
            }
        }
        res += "\n";
    }
    return res;
}

PseudoConfiguration PseudoConfiguration::from_v_rotations(int N,
                                                          const vector<vector<int>> &v_rotations,
                                                          const vector<Degree> &degrees) {
    assert(degrees.size() == (size_t)N);
    PseudoTriangulation pt = PseudoTriangulation::from_v_rotations(N, v_rotations);
    return PseudoConfiguration(N, pt.darts, degrees);
}

PseudoConfiguration PseudoConfiguration::disjoint_union(const PseudoConfiguration &L,
                                                        const PseudoConfiguration &R) {
    PseudoTriangulation pt = PseudoTriangulation::disjoint_union(
        static_cast<const PseudoTriangulation &>(L), static_cast<const PseudoTriangulation &>(R));
    vector<Degree> degrees = L.degrees;
    degrees.insert(degrees.end(), R.degrees.begin(), R.degrees.end());
    return PseudoConfiguration(pt.N, pt.darts, degrees);
}

vector<tuple<PseudoConfiguration, Mappings, Mappings>>
PseudoConfiguration::free_homomorphism(const PseudoConfiguration &pc0,
                                       const PseudoConfiguration &pc1, int dart_id0, int dart_id1) {
    PseudoConfiguration pc = disjoint_union(pc0, pc1);
    dart_id1 += pc0.darts.size();
    vector<pair<PseudoConfiguration, Mappings>> identified =
        pc.free_homomorphism({{dart_id0, dart_id1}});
    vector<tuple<PseudoConfiguration, Mappings, Mappings>> results;
    for (const auto &[identified_pc, mappings] : identified) {
        auto [vmap0, vmap1] = split_map(mappings.vmap, pc0.N);
        auto [dmap0, dmap1] = split_map(mappings.dmap, pc0.darts.size());
        Mappings mappings0(vmap0, dmap0);
        Mappings mappings1(vmap1, dmap1);
        results.emplace_back(identified_pc, mappings0, mappings1);
    }
    return results;
}

optional<pair<PseudoConfiguration, Mappings>>
PseudoConfiguration::dart_identification(const vector<pair<int, int>> &dart_pairs) const {
    auto [Z_star, mappings] = this->PseudoTriangulation::free_homomorphism(dart_pairs);
    if (Z_star.has_loop()) {
        return std::nullopt; // a loop error
    }

    vector<Degree> degrees_star(Z_star.N, Degree(1, INFTY));
    for (int v = 0; v < N; v++) {
        int v_star = mappings.vmap[v];
        if (Degree::disjoint(degrees_star[v_star], degrees[v])) {
            return std::nullopt; // a degree-mismatch error
        }
        degrees_star[v_star] = Degree::intersection(degrees_star[v_star], degrees[v]);
    }

    PseudoConfiguration pc = PseudoConfiguration(Z_star.N, Z_star.darts, degrees_star);
    return std::make_pair(pc, mappings);
}

vector<pair<PseudoConfiguration, Mappings>>
PseudoConfiguration::free_homomorphism(const vector<pair<int, int>> &dart_pairs) const {
    optional<pair<PseudoConfiguration, Mappings>> A = dart_identification(dart_pairs);
    if (!A.has_value()) return {};
    auto [Z_star, mappings]                              = A.value();
    vector<pair<PseudoConfiguration, Mappings>> Z_tildes = Z_star.resolve_degree_issues();
    vector<pair<PseudoConfiguration, Mappings>> results;
    for (const auto &[Z_tilde, mappings_tilde] : Z_tildes) {
        Mappings composed = mappings.compose(mappings_tilde);
        results.emplace_back(Z_tilde, composed);
    }
    return results;
}

vector<pair<PseudoConfiguration, Mappings>> PseudoConfiguration::resolve_degree_issues(void) const {
    vector<pair<PseudoConfiguration, Mappings>> Z;
    queue<pair<PseudoConfiguration, Mappings>> Q;
    Mappings initial_mappings = Mappings::initial_mappings(N, darts.size());
    Q.emplace(*this, initial_mappings);
    while (!Q.empty()) {
        auto [Z_tilde, mappings_tilde] = Q.front();
        Q.pop();
        if (Z_tilde.inner_subdegree_error()) continue;
        optional<int> v = Z_tilde.vertex_single_degree_issue();
        if (v.has_value()) {
            optional<pair<PseudoConfiguration, Mappings>> A =
                Z_tilde.fix_single_degree_issue(v.value());
            if (A.has_value()) {
                auto [Z_star, mappings_star] = A.value();
                Mappings composed            = mappings_tilde.compose(mappings_star);
                Q.emplace(Z_star, composed);
            }
            continue;
        }
        optional<pair<PseudoConfiguration, PseudoConfiguration>> B =
            Z_tilde.single_out_lower_degree();
        if (B.has_value()) {
            auto [Z1, Z2] = B.value();
            Q.emplace(Z1, mappings_tilde);
            Q.emplace(Z2, mappings_tilde);
            continue;
        }
        Z.emplace_back(Z_tilde, mappings_tilde);
    }
    return Z;
}

bool PseudoConfiguration::inner_subdegree_error(void) const {
    vector<int> n_incident   = this->n_incident_darts();
    vector<bool> is_boundary = this->is_boundary();
    for (int v = 0; v < N; v++) {
        if (!is_boundary[v] && n_incident[v] < degrees[v].lower) {
            return true;
        }
    }
    return false;
}

optional<int> PseudoConfiguration::vertex_single_degree_issue(void) const {
    vector<int> n_incident   = this->n_incident_darts();
    vector<bool> is_boundary = this->is_boundary();
    for (int v = 0; v < N; v++) {
        if (!degrees[v].fixed()) {
            continue;
        }
        if (degrees[v].lower < n_incident[v]) {
            return v;
        } else if (is_boundary[v] && n_incident[v] == degrees[v].lower) {
            return v;
        }
    }
    return std::nullopt;
}

optional<pair<PseudoConfiguration, Mappings>>
PseudoConfiguration::fix_single_degree_issue(int v) const {
    assert(degrees[v].fixed());
    vector<int> n_incident   = this->n_incident_darts();
    vector<bool> is_boundary = this->is_boundary();
    if (degrees[v].lower < n_incident[v]) {
        int e = is_boundary[v] ? first_dart(v) : any_dart(v);
        int f = this->suc_k_times(e, degrees[v].lower);
        return this->dart_identification({{e, f}});
    } else if (is_boundary[v] && n_incident[v] == degrees[v].lower) {
        optional<PseudoConfiguration> A = this->add_boundary_darts(v);
        if (A.has_value()) {
            return std::make_pair(A.value(), Mappings::initial_mappings(N, darts.size()));
        } else {
            return std::nullopt;
        }
    }
    assert(false);
}

optional<PseudoConfiguration> PseudoConfiguration::add_boundary_darts(int v) const {
    PseudoConfiguration Z = *this;
    int e_first           = Z.first_dart(v);
    int e_last            = Z.last_dart(v);
    int e_first_rev       = Z.darts[e_first].rev;
    int e_last_rev        = Z.darts[e_last].rev;
    int u                 = Z.darts[e_first_rev].head;
    int w                 = Z.darts[e_last_rev].head;
    if (u == w) {
        return std::nullopt; // a boundary error
    }
    int d_uw = Z.darts.size();
    int d_wu = d_uw + 1;
    Z.darts.emplace_back(u, d_wu, nil, e_first_rev); // (head, rev, succ, pred)
    Z.darts.emplace_back(w, d_uw, e_last_rev, nil);
    Z.darts[e_first].pred     = e_last;
    Z.darts[e_last].succ      = e_first;
    Z.darts[e_first_rev].succ = d_uw;
    Z.darts[e_last_rev].pred  = d_wu;
    return Z;
}

optional<pair<PseudoConfiguration, PseudoConfiguration>>
PseudoConfiguration::single_out_lower_degree(void) const {
    vector<int> n_incident = this->n_incident_darts();
    for (int v = 0; v < N; v++) {
        if (degrees[v].lower < degrees[v].upper && degrees[v].lower <= n_incident[v]) {
            PseudoConfiguration Z1 = *this, Z2 = *this;
            Z1.degrees[v].upper = this->degrees[v].lower;
            Z2.degrees[v].lower = this->degrees[v].lower + 1;
            return std::make_pair(Z1, Z2);
        }
    }
    return std::nullopt;
}

bool PseudoConfiguration::contain_conf(int center, const vector<Configuration> &confs) const {
    vector<vector<vector<int>>> darts_by_degree = this->darts_by_degree();
    for (size_t i = 0; i < confs.size(); i++) {
        const Configuration &conf = confs[i];
        const Dart &f             = conf.darts[conf.dart_id];
        int y                     = f.head;
        int x                     = conf.darts[f.rev].head;
        assert(conf.degrees[y].fixed());
        assert(conf.degrees[x].fixed());
        int d_y = conf.degrees[y].lower;
        int d_x = conf.degrees[x].lower;
        assert(d_y <= CONF_DEG_MAX);
        assert(d_x <= CONF_DEG_MAX);
        for (int f_star : darts_by_degree[d_y][d_x]) {
            if (d_y > 8 && darts[f_star].head != center) {
                continue;
            }
            if (this->rooted_contain_conf(f_star, conf)) {
                return true;
            }
        }
    }
    return false;
}

vector<vector<vector<int>>> PseudoConfiguration::darts_by_degree(void) const {
    vector<vector<vector<int>>> darts_by_degree(CONF_DEG_MAX + 1,
                                                vector<vector<int>>(CONF_DEG_MAX + 1));
    for (size_t i = 0; i < this->darts.size(); i++) {
        const Dart &e = this->darts[i];
        int y         = e.head;
        int x         = this->darts[e.rev].head;
        assert(this->degrees[y].fixed());
        assert(this->degrees[x].fixed());
        int d_y = this->degrees[y].lower;
        int d_x = this->degrees[x].lower;
        if (d_y > CONF_DEG_MAX || d_x > CONF_DEG_MAX) {
            continue;
        }
        darts_by_degree[d_y][d_x].push_back(i);
    }
    return darts_by_degree;
}

bool PseudoConfiguration::rooted_contain_conf(int dart_id, const Configuration &conf) const {
    return homomorphism(conf, conf.dart_id, *this, dart_id, Degree::include).has_value();
}

bool PseudoConfiguration::blocked_by_reducible_configuration(
    int center, const vector<Configuration> &confs) const {
    for (const PseudoConfiguration &Z : this->representative_degree(center)) {
        if (!Z.contain_conf(center, confs)) {
            return false;
        }
    }
    return true;
}

vector<PseudoConfiguration> PseudoConfiguration::representative_degree(int center) const {
    vector<Degree> any(this->N, Degree(1, INFTY));
    vector<vector<Degree>> T(1, any);
    for (int v = 0; v < N; v++) {
        vector<Degree> L;
        if (v == center && degrees[v].upper > CONF_DEG_MAX) {
            L.emplace_back(degrees[v].upper);
        } else if (v != center && degrees[v].upper > 8) {
            L.emplace_back(degrees[v].upper);
        } else {
            for (int deg = degrees[v].lower; deg <= degrees[v].upper; deg++) {
                L.emplace_back(deg);
            }
        }
        vector<vector<Degree>> new_T;
        for (const vector<Degree> &degs : T) {
            for (const Degree &d : L) {
                vector<Degree> new_degs = degs;
                new_degs[v]             = d;
                new_T.push_back(new_degs);
            }
        }
        T = new_T;
    }
    vector<PseudoConfiguration> results;
    for (const vector<Degree> &deg : T) {
        results.emplace_back(this->N, this->darts, deg);
    }
    return results;
}

bool PseudoConfiguration::always_apply(int dart_id, const Rule &rule) const {
    return homomorphism(rule, rule.st_id, *this, dart_id, Degree::include).has_value();
}

bool PseudoConfiguration::never_apply(int dart_id, const Rule &rule) const {
    return !homomorphism(rule, rule.st_id, *this, dart_id, Degree::has_intersection).has_value();
}

int PseudoConfiguration::amount_of_charge_send(int dart_id, const vector<Rule> &rules) const {
    int amount = 0;
    for (const Rule &rule : rules) {
        if (this->always_apply(dart_id, rule)) {
            amount += rule.amount;
        }
    }
    return amount;
}

int PseudoConfiguration::amount_of_possible_charge_send(
    int dart_id, const vector<CombinedRule> &combined_rules) const {
    int amount = 0;
    for (const Rule &combine_rule : combined_rules) {
        if (this->never_apply(dart_id, combine_rule)) {
            continue;
        }
        amount = std::max(amount, combine_rule.amount);
    }
    return amount;
}

bool PseudoConfiguration::dominantly_apply(int dart_id, const Rule &rule) const {
    std::function<bool(Degree, Degree)> g_dominant = [](const Degree &deg_r, const Degree &deg_c) {
        return Degree::has_intersection(deg_r, deg_c) &&
               (deg_r.upper == INFTY || deg_c.upper < CARTWHEEL_DEG_MAX);
    };
    return homomorphism(rule, rule.st_id, *this, dart_id, g_dominant).has_value();
}

vector<pair<PseudoConfiguration, Mappings>>
PseudoConfiguration::combine_each_cartwheel(int dart, const vector<CartWheel> &cartwheels,
                                            const vector<Configuration> &confs) const {
    vector<pair<PseudoConfiguration, Mappings>> Zs;
    for (const CartWheel &cartwheel : cartwheels) {
        for (int center_dart : cartwheel.center_darts) {
            vector<tuple<PseudoConfiguration, Mappings, Mappings>> free_homomorphisms =
                PseudoConfiguration::free_homomorphism(*this, cartwheel, dart, center_dart);
            for (const auto &[Z_star, mappings_pc, _] : free_homomorphisms) {
                if (Z_star.blocked_by_reducible_configuration(0, confs)) {
                    continue;
                }
                Zs.emplace_back(Z_star, mappings_pc);
            }
        }
    }
    return Zs;
}

vector<pair<PseudoConfiguration, Mappings>>
PseudoConfiguration::combine_each_cartwheel_twice(int dart1, int dart2,
                                                  const vector<CartWheel> &cartwheels,
                                                  const vector<Configuration> &confs) const {
    vector<pair<PseudoConfiguration, Mappings>> Z_stars =
        combine_each_cartwheel(dart1, cartwheels, confs);
    vector<pair<PseudoConfiguration, Mappings>> Z_star_stars;
    for (const auto &[Z_star, cw2Z_star] : Z_stars) {
        int mapped_dart2 = cw2Z_star.dmap[dart2];
        vector<pair<PseudoConfiguration, Mappings>> Zs =
            Z_star.combine_each_cartwheel(mapped_dart2, cartwheels, confs);
        for (const auto &[Z, Z_star2Z] : Zs) {
            Z_star_stars.emplace_back(Z, cw2Z_star.compose(Z_star2Z));
        }
    }
    return Z_star_stars;
}
