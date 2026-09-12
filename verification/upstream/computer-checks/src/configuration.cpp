#include "configuration.hpp"
#include <filesystem>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fstream>
#include <map>
#include <set>
#include <spdlog/spdlog.h>
#include <vector>
using std::map;
using std::set;
using std::vector;

// 1. internal vertex has clockwise rotations
// 2. ring are clockwisely ordered in 0,1,...,R-1
vector<Configuration> Configuration::from_file(const string &filename) {
    std::ifstream ifs(filename);
    if (!ifs) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    string dummy;
    std::getline(ifs, dummy);
    int N, R;
    ifs >> N >> R;
    vector<Degree> degrees(N, Degree(1, INFTY));
    vector<vector<int>> rotations(N);
    vector<vector<int>> suc(R, vector<int>(N, -1));
    for (int u = R; u < N; ++u) {
        int t;
        ifs >> t;
        assert(t == u + 1);
        int deg;
        ifs >> deg;
        degrees[u] = Degree(deg);
        for (int j = 0; j < deg; j++) {
            int v;
            ifs >> v;
            --v;
            rotations[u].push_back(v);
        }
        for (int j = 0; j < deg; j++) {
            int v   = rotations[u][j];
            int pre = rotations[u][(j + deg - 1) % deg];
            int nxt = rotations[u][(j + 1) % deg];
            if (v < R) {
                suc[v][nxt] = u;
                suc[v][u]   = pre;
            }
        }
    }
    // fix rotations of ring vertices
    for (int v = 0; v < R; v++) {
        int start = (v + 1) % R;
        int end   = (v + R - 1) % R;
        int curr  = start;
        while (curr != -1) {
            rotations[v].push_back(curr);
            curr = suc[v][curr];
        }
        if (rotations[v].back() != end) {
            throw std::runtime_error("Invalid configuration file: " + filename);
        }
        rotations[v].push_back(-1); // boundary
    }
    vector<Configuration> configurations = extend_from_cut_vertices(N, R, degrees, rotations);
    vector<Configuration> mirrors        = get_mirrors(configurations);
    configurations.insert(configurations.end(), mirrors.begin(), mirrors.end());
    return configurations;
}

vector<Configuration> Configuration::get_confs(const string &confdir) {
    vector<Configuration> confs;
    for (const auto &path : std::filesystem::directory_iterator(confdir)) {
        if (path.is_regular_file() && path.path().extension() == ".conf") {
            vector<Configuration> confs_in_file = Configuration::from_file(path.path().string());
            confs.insert(confs.end(), confs_in_file.begin(), confs_in_file.end());
        }
    }
    spdlog::info("Total {} configurations loaded.", confs.size());
    return confs;
}

Configuration Configuration::mirror(void) const {
    vector<Dart> D = darts;
    for (Dart &d : D) {
        std::swap(d.pred, d.succ);
    }
    return Configuration(dart_id, N, D, degrees);
}

vector<Configuration> get_mirrors(const vector<Configuration> &confs) {
    vector<Configuration> mirrors;
    for (const auto &conf : confs) {
        Configuration mirror = conf.mirror();
        mirrors.push_back(mirror);
    }
    return mirrors;
}

vector<Configuration> extend_from_cut_vertices(int N, int R, const vector<Degree> &degrees,
                                               const vector<vector<int>> &rotations) {
    vector<pair<int, int>> P = find_cut_pairs(N, R, rotations);
    // 2^|P|
    vector<Configuration> configurations;
    int P_size = P.size();
    if (P_size > 1) {
        spdlog::warn("Configuration has {} cut-vertices. This may cause a blow-up in the number "
                     "of configurations after handling cutvertices.",
                     P_size);
    }
    for (int S = 0; S < (1 << P_size); ++S) {
        vector<bool> remove(R, true);
        for (int i = 0; i < P_size; ++i) {
            auto [a, b] = P[i];
            if (S & (1 << i)) {
                remove[a] = false;
            } else {
                remove[b] = false;
            }
        }
        PseudoConfiguration Z = remove_ring(N, R, degrees, rotations, remove);
        int dart              = maximum_degree_dart(Z);
        Configuration conf(dart, Z.N, Z.darts, Z.degrees);
        configurations.push_back(conf);
    }
    return configurations;
}

vector<pair<int, int>> find_cut_pairs(int N, int R, const vector<vector<int>> &rotations) {
    vector<pair<int, int>> P;
    for (int i = R; i < N; ++i) {
        vector<int> U_R;
        int t = 0;
        int d = rotations[i].size();
        for (int j = 0; j < d; j++) {
            int k1 = rotations[i][j];
            assert(k1 != -1);
            if (k1 < R) { // k1 is a ring vertex
                U_R.push_back(k1);
            }
            int k2 = rotations[i][(j + 1) % d];
            if (k1 < R && k2 >= R) { // border between ring and internal vertex
                t++;
            }
        }
        assert(t <= (int)U_R.size());
        if (t >= 2 && U_R.size() != 2) {
            throw std::runtime_error(
                fmt::format("Invalid configuration (vertex {} is an invalid cut-vertex", i));
        }
        if (t == 2 && U_R.size() == 2) { // i is a cut vertex
            P.emplace_back(U_R[0], U_R[1]);
        }
    }
    return P;
}

PseudoConfiguration remove_ring(int N, int R, const vector<Degree> &degrees,
                                const vector<vector<int>> &rotations, const vector<bool> &remove) {
    // Step 1: assign new vertex IDs
    vector<int> old2new(N, -1);
    int new_id = 0;
    for (int i = 0; i < N; ++i) {
        if (i < R && remove[i]) {
            continue;
        }
        old2new[i] = new_id;
        new_id++;
    }
    int new_N = new_id;
    // Step 2: construct new rotations
    vector<vector<int>> new_rotations(new_N);
    for (int i = 0; i < N; ++i) {
        if (i < R && remove[i]) {
            continue;
        }
        for (int j : rotations[i]) {
            if (j == -1) {
                new_rotations[old2new[i]].push_back(-1);
            } else {
                new_rotations[old2new[i]].push_back(old2new[j]);
            }
        }
    }
    // Step 3: construct new degrees
    vector<Degree> new_degrees(new_N, Degree(1, INFTY));
    for (int i = 0; i < R; ++i) { // ring vertices
        if (remove[i]) {
            continue;
        }
        int k = old2new[i];
        int d = std::count_if(new_rotations[k].begin(), new_rotations[k].end(),
                              [](int v) { return v != -1; });
        assert(d == 3 || d == 4);
        new_degrees[k].lower = d + 1;
        new_degrees[k].upper = INFTY;
    }
    for (int i = R; i < N; ++i) {
        int k          = old2new[i];
        new_degrees[k] = degrees[i];
    }
    return PseudoConfiguration::from_v_rotations(new_N, new_rotations, new_degrees);
}

int maximum_degree_dart(const PseudoConfiguration &Z) {
    int f              = -1;
    pair<int, int> d_f = {0, 0};
    for (size_t i = 0; i < Z.darts.size(); i++) {
        int y = Z.darts[i].head;
        int x = Z.darts[Z.darts[i].rev].head;
        if (!Z.degrees[y].fixed() || !Z.degrees[x].fixed()) {
            continue;
        }
        pair<int, int> d_e = {Z.degrees[y].lower, Z.degrees[x].lower};
        if (d_e > d_f) {
            f   = i;
            d_f = d_e;
        }
    }
    assert(f != -1);
    return f;
}