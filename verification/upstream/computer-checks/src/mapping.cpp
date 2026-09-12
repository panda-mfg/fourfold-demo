#include "mapping.hpp"
#include <cassert>
#include <numeric>
#include <set>
using std::set;

Mappings Mappings::initial_mappings(int N, int dart_size) {
    vector<int> v_map(N);
    std::iota(v_map.begin(), v_map.end(), 0);
    vector<int> d_map(dart_size);
    std::iota(d_map.begin(), d_map.end(), 0);
    return Mappings(v_map, d_map);
}

Mappings Mappings::compose(const Mappings &other) const {
    Mappings composed(compose_map(this->vmap, other.vmap), compose_map(this->dmap, other.dmap));
    return composed;
}

vector<int> compose_map(const vector<int> &map1, const vector<int> &map2) {
    vector<int> composed_map(map1.size());
    for (size_t i = 0; i < map1.size(); i++) {
        composed_map[i] = map2[map1[i]];
    }
    return composed_map;
}

pair<vector<int>, vector<int>> split_map(const vector<int> &map, size_t l) {
    assert(0 <= l && l <= map.size());
    vector<int> left_map(l);
    vector<int> right_map(map.size() - l);
    for (size_t i = 0; i < l; i++) {
        left_map[i] = map[i];
    }
    for (size_t i = l; i < map.size(); i++) {
        right_map[i - l] = map[i];
    }
    return {left_map, right_map};
}
