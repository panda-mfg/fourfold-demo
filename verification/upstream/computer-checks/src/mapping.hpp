#ifndef MAPPING_HPP
#define MAPPING_HPP
#include <optional>
#include <utility>
#include <vector>
using std::optional;
using std::pair;
using std::vector;

struct Mappings {
    vector<int> vmap;
    vector<int> dmap;
    Mappings(const vector<int> &vmap = vector<int>(), const vector<int> &dmap = vector<int>())
        : vmap(vmap), dmap(dmap) {}

    static Mappings initial_mappings(int N, int dart_size);
    Mappings compose(const Mappings &other) const;
};

vector<int> compose_map(const vector<int> &map1, const vector<int> &map2);
pair<vector<int>, vector<int>> split_map(const vector<int> &map, size_t l);

#endif // MAPPING_HPP