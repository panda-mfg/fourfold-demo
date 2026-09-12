#include "cartwheel.hpp"
#include "configuration.hpp"
#include "degree.hpp"
#include "rule.hpp"
#include "util.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <filesystem>
#include <fmt/ranges.h>
#include <fstream>
#include <sstream>

string CartWheel::to_string(void) const {
    string res =
        fmt::format("center: {}, center_darts: {}\n", center, fmt::join(center_darts, ", "));
    res += PseudoConfiguration::to_string();
    return res;
}

CartWheel CartWheel::from_file(const string &filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        spdlog::critical("Failed to open cartwheel file for reading: {}", filename);
        throw std::runtime_error("Failed to open cartwheel file for reading: " + filename);
    }
    int N, center;
    ifs >> N >> center;
    --center;
    vector<Degree> degrees(N, Degree(1, INFTY));
    vector<vector<int>> rotation_vertices(N);
    for (int u = 0; u < N; u++) {
        int t;
        ifs >> t;
        assert(t == u + 1);
        int deg_lower, deg_upper;
        ifs >> deg_lower >> deg_upper;
        if (deg_upper == 0) {
            deg_upper = INFTY;
        }
        degrees[u] = Degree(deg_lower, deg_upper);

        string str;
        getline(ifs, str);
        boost::trim(str);
        std::stringstream ss(str);
        string v_str;
        while (getline(ss, v_str, ' ')) {
            int v = std::stoi(v_str);
            if (v != -1) {
                --v;
                assert(0 <= v && v < N);
            }
            rotation_vertices[u].push_back(v);
        }
    }
    PseudoConfiguration pc   = PseudoConfiguration::from_v_rotations(N, rotation_vertices, degrees);
    vector<int> center_darts = pc.get_e_rotations()[center];
    return CartWheel(center, center_darts, pc.N, pc.darts, pc.degrees);
}

void CartWheel::to_file(const string &filename) const {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        spdlog::critical("Failed to open cartwheel file for writing: {}", filename);
        throw std::runtime_error("Failed to open cartwheel file for writing: " + filename);
    }
    ofs << fmt::format("\n{} {}\n", N, center + 1);
    vector<vector<int>> e_rotations = this->get_e_rotations();
    for (int v = 0; v < N; v++) {
        ofs << fmt::format("{} {} {} ", v + 1, degrees[v].lower,
                           degrees[v].upper == INFTY ? 0 : degrees[v].upper);
        for (size_t i = 0; i < e_rotations[v].size(); i++) {
            int dart_id = e_rotations[v][i];
            if (dart_id == -1) {
                ofs << "-1 ";
            } else {
                int tail = darts[darts[dart_id].rev].head;
                ofs << tail + 1 << " ";
            }
        }
        ofs << "\n";
    }
    ofs.close();
    return;
}

vector<CartWheel> CartWheel::get_cartwheels(const string &cartwheeldir) {
    vector<CartWheel> cartwheels = get_objects<CartWheel>(cartwheeldir, ".cartwheel");
    spdlog::info("Total {} cartwheels loaded.", cartwheels.size());
    return cartwheels;
}

vector<CartWheel> CartWheel::enum_wheels(int center_degree) {
    vector<CartWheel> wheels;
    auto enum_degree = [&](auto &&enum_degree, vector<int> &degrees, int i, int i_lowest) -> void {
        if (i == center_degree) {
            if (!lex_min(degrees)) {
                return;
            }
            CartWheel wheel = CartWheel::generate_cartwheel(center_degree, degrees);
            wheels.push_back(wheel);
            return;
        }
        for (size_t j = i_lowest; j < CARTWHEEL_DEGREES_SIZE; j++) {
            degrees[i] = CARTWHEEL_DEGREES[j];
            enum_degree(enum_degree, degrees, i + 1, i_lowest);
        }
        return;
    };
    vector<int> degrees(center_degree);
    for (size_t j = 0; j < CARTWHEEL_DEGREES_SIZE; j++) {
        degrees[0] = CARTWHEEL_DEGREES[j];
        enum_degree(enum_degree, degrees, 1, j);
    }
    return wheels;
}

CartWheel CartWheel::generate_cartwheel(int d, const vector<int> &degrees) {
    assert((int)degrees.size() == d);
    vector<vector<int>> rotations(d + 1);
    for (int i = 1; i <= d; ++i) {
        rotations[0].push_back(i);
    }
    for (int i = 1; i <= d; ++i) {
        int i_next   = i < d ? i + 1 : 1;
        int i_prev   = i > 1 ? i - 1 : d;
        rotations[i] = {i_next, 0, i_prev};
    }
    int k = d + 1; // next vertex id to assign
    for (int i = 1; i <= d; ++i) {
        if (degrees[i - 1] == CARTWHEEL_DEG_MAX) { // 9
            continue;
        }
        int A = degrees[i - 1] - rotations[i].size(); // number of second neighbors added
        assert(A >= 0);
        for (int j = 0; j < A; j++) {
            int i_last = rotations[i].back();
            rotations.push_back({});
            rotations[k] = {i, i_last};
            rotations[i].push_back(k);
            rotations[i_last].insert(rotations[i_last].begin(), k);
            k++;
        }
        int i_first = rotations[i][0];
        int i_last  = rotations[i].back();
        rotations[i_first].push_back(i_last);
        rotations[i_last].insert(rotations[i_last].begin(), i_first);
    }
    for (int i = 1; i < k; ++i) {
        if (i > d || degrees[i - 1] == CARTWHEEL_DEG_MAX) { // 9
            rotations[i].push_back(-1);
        }
    }
    vector<Degree> all_degrees(
        k, Degree(CARTWHEEL_DEG_MIN, CARTWHEEL_DEG_MAX)); // [5, 9] for second neighbors
    all_degrees[0] = Degree(d);                           // center
    for (int i = 1; i <= d; ++i) {
        all_degrees[i] = Degree(degrees[i - 1]); // neighbors
    }
    PseudoConfiguration pc = PseudoConfiguration::from_v_rotations(k, rotations, all_degrees);
    vector<vector<int>> e_rotations = pc.get_e_rotations();
    return CartWheel(0, e_rotations[0], pc.N, pc.darts, pc.degrees);
}

vector<CartWheel> CartWheel::enum_possible_bad_wheels(int center_degree, const vector<Rule> &rules,
                                                      const vector<CombinedRule> &combined_rule,
                                                      const vector<Configuration> &confs) {
    vector<CartWheel> results;
    vector<CartWheel> all_wheels = CartWheel::enum_wheels(center_degree);
    int c                        = 0;
    for (const CartWheel &wheel : all_wheels) {
        spdlog::info("{}/{}", c++, all_wheels.size());
        if (wheel.prune({}, rules, combined_rule, confs)) {
            spdlog::debug("pruned by initial check");
            spdlog::debug("wheel:\n{}", wheel.to_string());
            continue;
        }
        results.push_back(wheel);
    }
    return results;
}

vector<pair<CartWheel, vector<CombinedRule>>>
CartWheel::fix_in_rules(const vector<Rule> &rules, const vector<CombinedRule> &combined_rules,
                        const vector<Configuration> &confs) const {
    int degree_center                                        = degrees[center].lower;
    vector<pair<CartWheel, vector<CombinedRule>>> cartwheels = {{*this, {}}};
    // Fix rules applied from neighbors to the center one by one, and prune in between.
    for (int i = 0; i < degree_center; i++) {
        spdlog::info("In {}/{} cartwheels.size(): {}", i, degree_center, cartwheels.size());
        vector<pair<CartWheel, vector<CombinedRule>>> new_cartwheels;
        for (const auto &[cartwheel, combined_rule_with_spokes] : cartwheels) {
            for (const CombinedRule &combined_rule : combined_rules) {
                vector<CartWheel> updated_cartwheels =
                    cartwheel.update_degree_by_rule(cartwheel.center_darts[i], combined_rule);
                for (const auto &updated_cartwheel : updated_cartwheels) {
                    vector<CombinedRule> updated_combined_rule_with_spokes =
                        combined_rule_with_spokes;
                    updated_combined_rule_with_spokes.push_back(combined_rule);
                    if (updated_cartwheel.prune(updated_combined_rule_with_spokes, rules,
                                                combined_rules, confs)) {
                        continue;
                    }
                    new_cartwheels.emplace_back(updated_cartwheel,
                                                updated_combined_rule_with_spokes);
                }
            }
        }
        cartwheels = new_cartwheels;
    }
    return cartwheels;
}

vector<CartWheel> CartWheel::update_degree_by_rule(int dart_id, const Rule &rule) const {
    optional<Mappings> rule2cw_opt = PseudoConfiguration::homomorphism(
        rule, rule.st_id, *this, dart_id, Degree::has_intersection);
    if (!rule2cw_opt.has_value()) {
        return {};
    }
    Mappings rule2cw            = rule2cw_opt.value();
    CartWheel updated_cartwheel = *this;
    for (int v_rule = 0; v_rule < rule.N; v_rule++) {
        int v_cw = rule2cw.vmap[v_rule];
        updated_cartwheel.degrees[v_cw] =
            Degree::intersection(updated_cartwheel.degrees[v_cw], rule.degrees[v_rule]);
    }
    return updated_cartwheel.concrete_degree_except_tail();
}

vector<CartWheel> CartWheel::concrete_degree_except_tail(void) const {
    vector<CartWheel> cartwheels = {*this};
    for (int v = 0; v < this->N; v++) {
        if (this->degrees[v].fixed() ||                    // already fixed degree
            this->degrees[v].upper == CARTWHEEL_DEG_MAX) { // tail degree range [d, 9]
            continue;
        }
        vector<CartWheel> new_cartwheels;
        for (size_t i = 0; i < CARTWHEEL_DEGREES_SIZE - 1; i++) {
            int d = CARTWHEEL_DEGREES[i];
            if (Degree::include(this->degrees[v], Degree(d))) {
                for (const CartWheel &cartwheel : cartwheels) {
                    CartWheel new_cartwheel  = cartwheel;
                    new_cartwheel.degrees[v] = Degree(d);
                    new_cartwheels.push_back(new_cartwheel);
                }
            }
        }
        cartwheels = new_cartwheels;
    }
    return cartwheels;
}

bool CartWheel::prune(const vector<CombinedRule> &combined_rule_with_spokes,
                      const vector<Rule> &rules, const vector<CombinedRule> &combined_rules,
                      const vector<Configuration> &confs) const {
    if (this->prune_by_non_associated_rule(combined_rule_with_spokes, rules)) {
        return true;
    }
    if (this->upper_bound_of_charge(combined_rule_with_spokes, rules, combined_rules) < 0) {
        return true;
    }
    if (this->blocked_by_reducible_configuration(center, confs)) {
        spdlog::debug("Pruned by reducible configuration");
        spdlog::debug("cartwheel:\n{}", this->to_string());
        return true;
    }
    return false;
}

bool CartWheel::prune_by_non_associated_rule(const vector<CombinedRule> &combined_rule_with_spokes,
                                             const vector<Rule> &rules) const {
    for (size_t j = 0; j < combined_rule_with_spokes.size(); j++) {
        for (size_t k = 0; k < rules.size(); k++) {
            assert(!combined_rule_with_spokes[j].combined_flag[k] ||
                   this->always_apply(this->center_darts[j], rules[k]));
            if (!combined_rule_with_spokes[j].combined_flag[k] &&
                this->always_apply(this->center_darts[j], rules[k])) {
                spdlog::debug("Pruned by nonassociated rule");
                spdlog::debug("cartwheel:\n{}", this->to_string());
                return true;
            }
        }
    }
    return false;
}

int CartWheel::upper_bound_of_charge(const vector<CombinedRule> &combined_rule_with_spokes,
                                     const vector<Rule> &rules,
                                     const vector<CombinedRule> &combined_rules) const {
    size_t degree_center = degrees[center].lower;
    int in_charge_sum    = 0;
    for (size_t j = 0; j < combined_rule_with_spokes.size(); j++) {
        in_charge_sum += combined_rule_with_spokes[j].amount;
    }
    for (size_t j = combined_rule_with_spokes.size(); j < degree_center; j++) {
        in_charge_sum += amount_of_possible_charge_send(center_darts[j], combined_rules);
    }
    int out_charge_sum = 0;
    for (size_t i = 0; i < degree_center; i++) {
        int from_center = darts[center_darts[i]].rev;
        out_charge_sum += amount_of_charge_send(from_center, rules);
    }
    int initial_charge = 10 * (6 - degree_center);
    return initial_charge - out_charge_sum + in_charge_sum;
}

vector<pair<CartWheel, vector<CombinedRule>>>
CartWheel::fix_out_rules(const vector<pair<CartWheel, vector<CombinedRule>>> &cartwheels_in_fixed,
                         const vector<Rule> &rules, const vector<CombinedRule> &combined_rules,
                         const vector<Configuration> &confs) const {
    int degree_center = degrees[center].lower;
    std::queue<pair<CartWheel, vector<CombinedRule>>> que;
    for (const auto &[cartwheel, combined_rule_with_spokes] : cartwheels_in_fixed) {
        que.emplace(cartwheel, combined_rule_with_spokes);
    }
    vector<pair<CartWheel, vector<CombinedRule>>> cartwheels;
    while (!que.empty()) {
        auto [cartwheel, combined_rule_with_spokes] = que.front();
        que.pop();
        bool refined_flag = false;
        for (int i = 0; i < degree_center; i++) {
            for (const Rule &rule : rules) {
                if (!cartwheel.should_refine(i, rule)) {
                    continue;
                }
                refined_flag              = true;
                vector<CartWheel> refined = cartwheel.refinement(i, rule);
                int n_added               = 0;
                for (const CartWheel &refined_cartwheel : refined) {
                    if (refined_cartwheel.prune(combined_rule_with_spokes, rules, combined_rules,
                                                confs)) {
                        continue;
                    }
                    que.emplace(refined_cartwheel, combined_rule_with_spokes);
                    ++n_added;
                }
                spdlog::info("Num refined: {}, Num enqueued: {}, Size (Queue + Results): {}, Size "
                             "(Queue): {}",
                             refined.size(), n_added, que.size() + cartwheels.size(), que.size());
                break;
            }
            if (refined_flag) {
                break;
            }
        }
        if (!refined_flag) {
            cartwheels.emplace_back(cartwheel, combined_rule_with_spokes);
        }
    }
    return cartwheels;
}

bool CartWheel::should_refine(int i, const Rule &rule) const {
    int from_center = darts[center_darts[i]].rev;
    return !this->always_apply(from_center, rule) && this->dominantly_apply(from_center, rule);
}

vector<CartWheel> CartWheel::refinement(int i, const Rule &rule) const {
    int from_center = darts[center_darts[i]].rev;
    Mappings rule2cw =
        this->homomorphism(rule, rule.st_id, *this, from_center, Degree::has_intersection).value();
    vector<int> U_R;
    for (int v_rule = 0; v_rule < rule.N; v_rule++) {
        int v_cw = rule2cw.vmap[v_rule];
        if (this->degrees[v_cw].upper == CARTWHEEL_DEG_MAX && // 9
            this->degrees[v_cw].lower < rule.degrees[v_rule].lower) {
            assert(rule.degrees[v_rule].upper == INFTY);
            U_R.push_back(v_rule);
        }
    }
    assert(U_R.size() > 0);
    CartWheel C_always        = this->refine_always(U_R, rule2cw, rule);
    vector<CartWheel> C_never = this->refine_never(U_R, rule2cw, rule);
    C_never.push_back(C_always);
    return C_never;
}

CartWheel CartWheel::refine_always(const vector<int> &U_R, const Mappings &rule2cw,
                                   const Rule &rule) const {
    CartWheel C_always = *this;
    for (int v_rule : U_R) {
        int v_cw = rule2cw.vmap[v_rule];
        assert(rule.degrees[v_rule].upper == INFTY);
        assert(this->degrees[v_cw].upper == CARTWHEEL_DEG_MAX); // 9
        C_always.degrees[v_cw].lower = rule.degrees[v_rule].lower;
    }
    return C_always;
}

vector<CartWheel> CartWheel::refine_never(const vector<int> &U_R, const Mappings &rule2cw,
                                          const Rule &rule) const {
    vector<CartWheel> C_never;
    for (int v_rule : U_R) {
        int v_cw = rule2cw.vmap[v_rule];
        assert(rule.degrees[v_rule].upper == INFTY);
        assert(this->degrees[v_cw].upper == CARTWHEEL_DEG_MAX); // 9
        CartWheel base              = *this;
        base.degrees[v_cw].upper    = rule.degrees[v_rule].lower - 1;
        vector<CartWheel> concretes = base.concrete_degree_except_tail();
        C_never.insert(C_never.end(), concretes.begin(), concretes.end());
    }
    return C_never;
}

vector<CartWheel> CartWheel::enum_bad_cartwheels(const vector<Rule> &rules,
                                                 const vector<CombinedRule> &combined_rules,
                                                 const vector<Configuration> &confs) const {
    vector<pair<CartWheel, vector<CombinedRule>>> cartwheels_in_fixed =
        this->fix_in_rules(rules, combined_rules, confs);
    spdlog::info("cartwheel_in_fixed.size(): {}", cartwheels_in_fixed.size());
    vector<pair<CartWheel, vector<CombinedRule>>> cartwheels_fixed =
        this->fix_out_rules(cartwheels_in_fixed, rules, combined_rules, confs);
    vector<CartWheel> cartwheels;
    for (const auto &[cartwheel, combined_rule_with_spokes] : cartwheels_fixed) {
        int C = cartwheel.upper_bound_of_charge(combined_rule_with_spokes, rules, combined_rules);
        int d = cartwheel.degrees[cartwheel.center].lower;
        array<vector<int>, CARTWHEEL_DEG_MAX + 1> darts_by_deg = cartwheel.center_darts_by_degree();
        assert(C == 0);
        assert(d == 7 || d == 8);
        assert(darts_by_deg[7].size() + darts_by_deg[8].size() + darts_by_deg[9].size() > 0);
        cartwheels.push_back(cartwheel);
    }
    return cartwheels;
}

array<vector<int>, CARTWHEEL_DEG_MAX + 1> CartWheel::center_darts_by_degree(void) const {
    array<vector<int>, CARTWHEEL_DEG_MAX + 1> center_darts_by_degree;
    for (int dart_id : center_darts) {
        int neighbor = darts[darts[dart_id].rev].head;
        int deg      = degrees[neighbor].lower;
        assert(CARTWHEEL_DEG_MIN <= deg && deg <= CARTWHEEL_DEG_MAX);
        center_darts_by_degree[deg].push_back(dart_id);
    }
    return center_darts_by_degree;
}

void run_enum_wheels(int center_degree, const string &confdir, const string &ruledir,
                     const string &combined_ruledir, const string &outdir) {
    vector<Configuration> confs         = Configuration::get_confs(confdir);
    vector<Rule> rules                  = Rule::get_rules(ruledir);
    vector<CombinedRule> combined_rules = CombinedRule::get_combined_rules(combined_ruledir);
    vector<CartWheel> wheels =
        CartWheel::enum_possible_bad_wheels(center_degree, rules, combined_rules, confs);
    spdlog::info("Generated {} wheels.", wheels.size());
    for (size_t i = 0; i < wheels.size(); i++) {
        string filename = fmt::format("{}/d{}_{}.cartwheel", outdir, center_degree, i);
        wheels[i].to_file(filename);
    }
    return;
}

void run_enum_cartwheels(const string &wheel_file, const string &confdir, const string &ruledir,
                         const string &combined_ruledir, const string &outdir) {
    CartWheel cartwheel                 = CartWheel::from_file(wheel_file);
    vector<Configuration> confs         = Configuration::get_confs(confdir);
    vector<Rule> rules                  = Rule::get_rules(ruledir);
    vector<CombinedRule> combined_rules = CombinedRule::get_combined_rules(combined_ruledir);
    vector<CartWheel> enumed_wheels = cartwheel.enum_bad_cartwheels(rules, combined_rules, confs);
    spdlog::info("Total {} cartwheels after enumerating degrees.", enumed_wheels.size());
    string wheel_basename = std::filesystem::path(wheel_file).stem().string();
    for (size_t i = 0; i < enumed_wheels.size(); i++) {
        string filename = fmt::format("{}/{}_{}.cartwheel", outdir, wheel_basename, i);
        enumed_wheels[i].to_file(filename);
    }
    return;
}
