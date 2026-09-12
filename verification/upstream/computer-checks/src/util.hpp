#ifndef UTIL_HPP
#define UTIL_HPP
#include <algorithm>
#include <cassert>
#include <concepts>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>
using std::map;
using std::optional;
using std::pair;
using std::set;
using std::string;
using std::vector;
namespace fs = std::filesystem;

struct Unionfind {
    int n;
    vector<int> parents;

    Unionfind(int n) : n(n), parents(n, -1) {}

    int root(int x) {
        if (parents[x] < 0) return x;
        return root(parents[x]);
    }

    void unite(int x, int y) {
        x = root(x);
        y = root(y);

        if (x == y) return;
        parents[x] = y;
        return;
    }

    bool same(int x, int y) { return root(x) == root(y); }

    vector<int> each_root(void) {
        vector<int> each_root(n);
        for (int i = 0; i < n; i++) {
            each_root[i] = this->root(i);
        }
        return each_root;
    }

    vector<int> all_roots(void) {
        vector<int> all_roots;
        for (int i = 0; i < n; i++) {
            if (parents[i] < 0) {
                all_roots.push_back(i);
            }
        }
        return all_roots;
    }

    vector<int> index_roots(void) {
        vector<int> index_map(n, -1);
        int index = 0;
        for (int i = 0; i < n; i++) {
            if (parents[i] < 0) {
                index_map[i] = index++;
            }
        }
        return index_map;
    }

    int num_roots(void) {
        int n_roots = 0;
        for (int i = 0; i < n; i++) {
            if (parents[i] < 0) n_roots++;
        }
        return n_roots;
    }
};

// Check if `a` is lexicographically minimum among its rotations
template <typename T> inline bool lex_min(const vector<T> &a) {
    vector<T> rotated = a;
    for (size_t i = 0; i < a.size(); i++) {
        std::rotate(rotated.begin(), rotated.begin() + 1, rotated.end());
        if (rotated < a) {
            return false;
        }
    }
    return true;
}

template <typename T>
concept HasFromFile = requires(string filename) {
    { T::from_file(filename) } -> std::same_as<T>;
};

// get objects from dir with given extension, sorted by filename
template <HasFromFile T> vector<T> get_objects(const string &dir, const string &extension) {
    set<fs::directory_entry> ordered_paths;
    for (const auto &path : fs::directory_iterator(dir)) {
        if (path.is_regular_file() && path.path().extension() == extension) {
            ordered_paths.insert(path);
        }
    }
    vector<T> objects;
    for (const auto &path : ordered_paths) {
        spdlog::debug("Loading from file: {}", path.path().string());
        T obj = T::from_file(path.path().string());
        objects.push_back(obj);
    }
    return objects;
}

#endif // UTIL_HPP
