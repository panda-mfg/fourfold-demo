#include "rule.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <spdlog/spdlog.h>
#include <sstream>
namespace fs = std::filesystem;

string Rule::to_string(void) const {
    string res = fmt::format("st_id: {}, amount: {}\n", st_id, amount);
    res += PseudoConfiguration::to_string();
    return res;
}

Rule Rule::read(std::ifstream &ifs) {
    string dummy;
    std::getline(ifs, dummy);
    int N, s, t, amount;
    ifs >> N >> s >> t >> amount;
    --s;
    --t;
    vector<Degree> degrees(N, Degree(1, INFTY));
    vector<vector<int>> rotation_vertices(N);
    for (int u = 0; u < N; ++u) {
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
    PseudoConfiguration pc = PseudoConfiguration::from_v_rotations(N, rotation_vertices, degrees);
    vector<int> st         = pc.get_darts(t, s);
    assert(st.size() == 1);
    return Rule(st[0], amount, pc.N, pc.darts, pc.degrees);
}

void Rule::write(std::ofstream &ofs) const {
    ofs << fmt::format("\n{} {} {} {}\n", N, darts[darts[st_id].rev].head + 1,
                       darts[st_id].head + 1, amount);
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
    return;
}

Rule Rule::from_file(const string &filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        spdlog::critical("Failed to open rule file: {}", filename);
        throw std::runtime_error("Failed to open rule file: " + filename);
    }
    return read(ifs);
}

void Rule::to_file(const string &filename) const {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        spdlog::critical("Failed to open rule file for writing: {}", filename);
        throw std::runtime_error("Failed to open rule file for writing: " + filename);
    }
    this->write(ofs);
    ofs.close();
    return;
}

// get rules from ruledir, sorted by filename
vector<Rule> Rule::get_rules(const string &ruledir) {
    vector<Rule> rules = get_objects<Rule>(ruledir, ".rule");
    spdlog::info("Total {} rules loaded.", rules.size());
    return rules;
}

CombinedRule CombinedRule::from_file(const string &filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        spdlog::critical("Failed to open rule file: {}", filename);
        throw std::runtime_error("Failed to open rule file: " + filename);
    }
    Rule rule = Rule::read(ifs);
    string line;
    ifs >> line;
    vector<bool> combined_flag;
    for (char c : line) {
        if (c == '0') {
            combined_flag.push_back(false);
        } else if (c == '1') {
            combined_flag.push_back(true);
        } else {
            spdlog::critical("Invalid combined flag in file {}: {}", filename, line);
            throw std::runtime_error("Invalid combined flag in file " + filename + ": " + line);
        }
    }
    return CombinedRule(combined_flag, rule.st_id, rule.amount, rule.N, rule.darts, rule.degrees);
}

void CombinedRule::to_file(const string &filename) const {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        spdlog::critical("Failed to open rule file for writing: {}", filename);
        throw std::runtime_error("Failed to open rule file for writing: " + filename);
    }
    this->write(ofs);
    for (bool flag : combined_flag) {
        ofs << (flag ? '1' : '0');
    }
    ofs << "\n";
    ofs.close();
    return;
}

vector<CombinedRule> CombinedRule::get_combined_rules(const string &combined_ruledir) {
    vector<CombinedRule> combined_rules =
        get_objects<CombinedRule>(combined_ruledir, ".combined_rule");
    spdlog::info("Total {} combined rules loaded.", combined_rules.size());
    return combined_rules;
}

// genrate combined rules from `combination` and `rules[i]`.
vector<CombinedRule>
CombinedRule::add_rule_to_combination(const vector<Rule> &rules, size_t i,
                                      const vector<Configuration> &confs) const {
    vector<tuple<PseudoConfiguration, Mappings, Mappings>> Z_tildes =
        PseudoConfiguration::free_homomorphism(*this, rules[i], this->st_id, rules[i].st_id);
    vector<bool> new_combined_flag = this->combined_flag;
    new_combined_flag[i]           = true;
    vector<CombinedRule> R_tildes;
    for (const auto &[Z_tilde, mappings_combination, _] : Z_tildes) {
        R_tildes.push_back(CombinedRule(new_combined_flag, mappings_combination.dmap[this->st_id],
                                        this->amount + rules[i].amount, Z_tilde.N, Z_tilde.darts,
                                        Z_tilde.degrees));
    }
    if (confs.empty()) {
        return R_tildes;
    }
    vector<CombinedRule> R_stars;
    for (const CombinedRule &R_tilde : R_tildes) {
        if (R_tilde.blocked_by_reducible_configuration(R_tilde.darts[R_tilde.st_id].head, confs)) {
            continue;
        }
        R_stars.push_back(R_tilde);
    }
    return R_stars;
}

vector<CombinedRule> combine_rules(const vector<Rule> &rules, const vector<Configuration> &confs) {
    vector<bool> default_combined_flag(rules.size(), false);
    CombinedRule Z0 =
        CombinedRule(default_combined_flag, 0, 0, 2, {Dart(0, 1, -1, -1), Dart(1, 0, -1, -1)},
                     {Degree(1, INFTY), Degree(1, INFTY)});
    vector<CombinedRule> combined_rules = {Z0};
    for (size_t i = 0; i < rules.size(); ++i) {
        vector<CombinedRule> next_combined_rules = combined_rules;
        for (const auto &combination : combined_rules) {
            vector<CombinedRule> new_combinations =
                combination.add_rule_to_combination(rules, i, confs);
            next_combined_rules.insert(next_combined_rules.end(), new_combinations.begin(),
                                       new_combinations.end());
        }
        combined_rules = next_combined_rules;
    }
    int max_amount = 0;
    for (const auto &combined_rule : combined_rules) {
        max_amount = std::max(max_amount, combined_rule.amount);
    }
    spdlog::info("Generated {} combined rules.", combined_rules.size());
    spdlog::info("Max amount among combined rules: {}", max_amount);
    return combined_rules;
}

void run_combine_rules(const string &confdir, const string &ruledir, const string &outdir) {
    vector<Configuration> confs         = Configuration::get_confs(confdir);
    vector<Rule> rules                  = Rule::get_rules(ruledir);
    vector<CombinedRule> combined_rules = combine_rules(rules, confs);
    for (size_t i = 0; i < combined_rules.size(); ++i) {
        string filename = fmt::format("{}/combined_rule_{}.combined_rule", outdir, i + 1);
        combined_rules[i].to_file(filename);
    }
    return;
}
