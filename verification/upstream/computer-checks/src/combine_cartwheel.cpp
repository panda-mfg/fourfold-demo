#include "combine_cartwheel.hpp"
#include "cartwheel.hpp"
#include "configuration.hpp"
#include <boost/asio.hpp>
#include <boost/thread.hpp>

vector<CartWheel> delete_degree_from_k_to_9(const vector<CartWheel> &cartwheels, int k) {
    vector<CartWheel> new_cartwheels;
    for (CartWheel cw : cartwheels) {
        bool remove = false;
        for (int v = 0; v < cw.N; v++) {
            if (cw.degrees[v].lower == k && cw.degrees[v].upper == k) {
                remove = true;
                break;
            } else if (cw.degrees[v].lower == k - 1 &&
                       cw.degrees[v].upper == CARTWHEEL_DEG_MAX) { // [k-1, 9]
                cw.degrees[v].upper = k - 1;
            }
        }
        if (!remove) {
            new_cartwheels.push_back(cw);
        }
    }
    return new_cartwheels;
}

vector<CartWheel> delete_7triangle(const vector<CartWheel> &cartwheels) {
    vector<CartWheel> new_cartwheels;
    vector<Configuration> confs = {get_7triangle()};
    for (const CartWheel &cartwheel : cartwheels) {
        if (cartwheel.blocked_by_reducible_configuration(0, confs)) {
            continue;
        }
        new_cartwheels.push_back(cartwheel);
    }
    return new_cartwheels;
}

Configuration get_7triangle(void) {
    PseudoConfiguration T7 = PseudoConfiguration::from_v_rotations(
        3, {{1, 2, -1}, {2, 0, -1}, {0, 1, -1}}, {Degree(7), Degree(7), Degree(7)});
    return Configuration(0, 3, T7.darts, T7.degrees);
}

// A vertex of degree 8
void run_check_deg8(const string &cartwheeldir, const string &confdir) {
    vector<CartWheel> cartwheels = CartWheel::get_cartwheels(cartwheeldir);
    vector<Configuration> confs  = Configuration::get_confs(confdir);
    check_deg8(cartwheels, confs);
    return;
}

void check_deg8(const vector<CartWheel> &all_cartwheels, const vector<Configuration> &confs) {
    vector<CartWheel> cartwheels = delete_degree_from_k_to_9(all_cartwheels, 9);
    spdlog::info("After removing cartwheels with degree 9,");
    spdlog::info("{} cartwheels remain.", cartwheels.size());
    unsigned int num_threads = boost::thread::hardware_concurrency();
    boost::asio::thread_pool pool(num_threads);
    for (const auto &cartwheel : cartwheels) {
        boost::asio::post(pool, [&]() {
            if (cartwheel.degrees[cartwheel.center] == Degree(8)) {
                array<vector<int>, CARTWHEEL_DEG_MAX + 1> center_darts =
                    cartwheel.center_darts_by_degree();
                if (center_darts[8].size() > 0) {
                    check88(cartwheel, center_darts[8], cartwheels, confs);
                } else if (center_darts[7].size() == 1) {
                    check87(cartwheel, center_darts[7], cartwheels, confs);
                } else if (center_darts[7].size() > 1) {
                    check787(cartwheel, center_darts[7], cartwheels, confs);
                } else {
                    assert(false);
                }
            }
        });
    }
    spdlog::info("All cases have been posted to the thread pool.");
    pool.join();
    spdlog::info("Finished checking degree 8 vertices.");
    return;
}

void check88(const CartWheel &cartwheel, const vector<int> &darts8,
             const vector<CartWheel> &cartwheels, const vector<Configuration> &confs) {
    for (int dart : darts8) {
        int rev = cartwheel.darts[dart].rev;
        vector<pair<PseudoConfiguration, Mappings>> combined =
            cartwheel.combine_each_cartwheel(rev, cartwheels, confs);
        assert(combined.size() == 0);
    }
    return;
}

void check87(const CartWheel &cartwheel, const vector<int> &darts7,
             const vector<CartWheel> &cartwheels, const vector<Configuration> &confs) {
    assert(darts7.size() == 1);
    int dart = darts7[0];
    int rev  = cartwheel.darts[dart].rev;
    vector<pair<PseudoConfiguration, Mappings>> combined =
        cartwheel.combine_each_cartwheel(rev, cartwheels, confs);
    assert(combined.size() == 0);
    return;
}

void check787(const CartWheel &cartwheel, const vector<int> &darts7,
              const vector<CartWheel> &cartwheels, const vector<Configuration> &confs) {
    int min_dist = INFTY;
    vector<int> dist(darts7.size(), 0);
    for (size_t i = 0; i < darts7.size(); i++) {
        int dart1 = darts7[i];
        int dart2 = i == darts7.size() - 1 ? darts7[0] : darts7[i + 1];
        while (dart1 != dart2) {
            dart1 = cartwheel.darts[dart1].succ;
            dist[i]++;
        }
        min_dist = std::min(min_dist, dist[i]);
    }
    for (size_t i = 0; i < darts7.size(); i++) {
        if (dist[i] > min_dist) {
            continue;
        }
        int dart1 = darts7[i];
        int dart2 = i == darts7.size() - 1 ? darts7[0] : darts7[i + 1];
        int rev1  = cartwheel.darts[dart1].rev;
        int rev2  = cartwheel.darts[dart2].rev;
        vector<pair<PseudoConfiguration, Mappings>> combined_set =
            cartwheel.combine_each_cartwheel_twice(rev1, rev2, cartwheels, confs);
        for (const auto &[combined, mappings_cw] : combined_set) {
            int center = mappings_cw.vmap[cartwheel.center];
            assert(containX(combined, center));
        }
    }
    return;
}

PseudoConfiguration getX(void) {
    const PseudoConfiguration X = PseudoConfiguration::from_v_rotations(
        17,
        {{1, 2, 3, 4, 5, 6, 7, 8},
         {0, 8, 11, 12, 2},
         {0, 1, 12, -1, 3},
         {0, 2, -1, 13, 4},
         {0, 3, 13, 14, 5},
         {0, 4, 14, 15, 16, -1, 6},
         {0, 5, -1, 7},
         {0, 6, -1, 8},
         {0, 7, -1, 9, 10, 11, 1},
         {8, -1, 10},
         {8, 9, -1, 11},
         {1, 8, 10, -1, 12},
         {1, 11, -1, 2},
         {3, -1, 14, 4},
         {4, 13, -1, 15, 5},
         {5, 14, -1, 16},
         {5, 15, -1}},
        {Degree(8), Degree(5), Degree(5), Degree(5), Degree(5), Degree(7), Degree(5), Degree(5),
         Degree(7), Degree(5), Degree(5), Degree(8), Degree(5), Degree(5), Degree(8), Degree(5),
         Degree(5)});
    return X;
}

bool containX(const PseudoConfiguration &Z, int v) {
    PseudoConfiguration X = getX();
    int dart_Z            = Z.any_dart(v);
    int dart_X            = X.any_dart(0);
    for (int i = 0; i < 8; i++) {
        if (PseudoConfiguration::homomorphism(X, dart_X, Z, dart_Z, Degree::include).has_value()) {
            return true;
        }
        dart_X = X.darts[dart_X].succ;
    }
    return false;
}

// 7-triangle
void run_check_7triangle(const string &cartwheeldir, const string &confdir) {
    vector<CartWheel> cartwheels = CartWheel::get_cartwheels(cartwheeldir);
    vector<Configuration> confs  = Configuration::get_confs(confdir);
    check_7triangle(cartwheels, confs);
    return;
}

void check_7triangle(const vector<CartWheel> &all_cartwheels, const vector<Configuration> &confs) {
    vector<CartWheel> cartwheels = delete_degree_from_k_to_9(all_cartwheels, 9);
    cartwheels                   = delete_degree_from_k_to_9(cartwheels, 8);
    spdlog::info("After removing cartwheels with degree 8 and 9,");
    spdlog::info("{} cartwheels remain.", cartwheels.size());
    unsigned int num_threads = boost::thread::hardware_concurrency();
    boost::asio::thread_pool pool(num_threads);
    for (const auto &cartwheel : cartwheels) {
        boost::asio::post(pool, [&]() {
            for (int e : cartwheel.center_darts) {
                int f     = cartwheel.darts[e].succ;
                int rev_e = cartwheel.darts[e].rev;
                int rev_f = cartwheel.darts[f].rev;
                int v_e   = cartwheel.darts[rev_e].head;
                int v_f   = cartwheel.darts[rev_f].head;
                assert(cartwheel.degrees[v_e].lower == cartwheel.degrees[v_e].upper);
                assert(cartwheel.degrees[v_f].lower == cartwheel.degrees[v_f].upper);
                if (cartwheel.degrees[v_e].lower == 7 && cartwheel.degrees[v_f].lower == 7) {
                    vector<pair<PseudoConfiguration, Mappings>> combined =
                        cartwheel.combine_each_cartwheel_twice(rev_e, rev_f, cartwheels, confs);
                    assert(combined.size() == 0);
                }
            }
        });
    }
    spdlog::info("All cases have been posted to the thread pool.");
    pool.join();
    spdlog::info("Finished checking 7-triangles.");
    return;
}

// A vertex of degree 7
void run_check_deg7(const string &cartwheeldir, const string &confdir) {
    vector<CartWheel> cartwheels = CartWheel::get_cartwheels(cartwheeldir);
    vector<Configuration> confs  = Configuration::get_confs(confdir);
    check_deg7(cartwheels, confs);
    return;
}

void check_deg7(const vector<CartWheel> &all_cartwheels, vector<Configuration> &confs) {
    vector<CartWheel> cartwheels = delete_degree_from_k_to_9(all_cartwheels, 9);
    cartwheels                   = delete_degree_from_k_to_9(cartwheels, 8);
    cartwheels                   = delete_7triangle(cartwheels);
    spdlog::info(
        "After removing cartwheels with degree 8 and 9 and cartwheels containing a 7-triangle,");
    spdlog::info("{} cartwheels remain.", cartwheels.size());
    confs.push_back(get_7triangle());
    unsigned int num_threads = boost::thread::hardware_concurrency();
    boost::asio::thread_pool pool(num_threads);
    for (const auto &cartwheel : cartwheels) {
        boost::asio::post(pool, [&]() {
            array<vector<int>, CARTWHEEL_DEG_MAX + 1> center_darts =
                cartwheel.center_darts_by_degree();
            if (center_darts[7].size() == 1) {
                check77(cartwheel, center_darts[7], cartwheels, confs);
            } else if (center_darts[7].size() > 1) {
                check777(cartwheel, center_darts[7], cartwheels, confs);
            } else {
                assert(false);
            }
        });
    }
    spdlog::info("All cases have been posted to the thread pool.");
    pool.join();
    spdlog::info("Finished checking degree 7 vertices.");
    return;
}

void check77(const CartWheel &cartwheel, const vector<int> &darts7,
             const vector<CartWheel> &cartwheels, const vector<Configuration> &confs) {
    assert(darts7.size() == 1);
    int dart = darts7[0];
    int rev  = cartwheel.darts[dart].rev;
    vector<pair<PseudoConfiguration, Mappings>> combined =
        cartwheel.combine_each_cartwheel(rev, cartwheels, confs);
    assert(combined.size() == 0);
    return;
}

void check777(const CartWheel &cartwheel, const vector<int> &darts7,
              const vector<CartWheel> &cartwheels, const vector<Configuration> &confs) {
    for (size_t i = 0; i < darts7.size(); i++) {
        int e1   = darts7[i];
        int rev1 = cartwheel.darts[e1].rev;
        for (size_t j = 0; j < i; j++) {
            int e2   = darts7[j];
            int rev2 = cartwheel.darts[e2].rev;
            vector<pair<PseudoConfiguration, Mappings>> combined =
                cartwheel.combine_each_cartwheel_twice(rev1, rev2, cartwheels, confs);
            assert(combined.size() == 0);
        }
    }
    return;
}
