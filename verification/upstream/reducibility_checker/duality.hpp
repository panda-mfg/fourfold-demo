#include <fstream>
#include <iostream>
#include <string>
#include <set>
#include <map>
#include <functional>
#include <tuple>
#include <vector>
#include <cassert>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

using std::ifstream;
using std::ostream;
using std::string;
using std::set;
using std::map;
using std::pair;
using std::tuple;
using std::vector;

// 主グラフの辺情報をもとに双対となる 3 正則グラフを返す
vector<tuple<int, int, int>> GetDuality(int N, int R, set<pair<int, int>>& primalEdges) {
    auto is3Cycle = [&] (int x, int y, int z) {
        return primalEdges.count({x, y}) && primalEdges.count({y, z}) && primalEdges.count({z, x});
    };
    set<tuple<int, int, int>> triangles;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < i; j++) {
            for (int k = 0; k < j; k++) {
                if (is3Cycle(k, j, i)) {
                    spdlog::trace("3c: [{}, {}, {}]", k, j, i);
                    triangles.insert({k, j, i});
                }
            }
        }
    }
    map<pair<int, int>, int> indexOfEdge;
    int counter = 0;
    auto addEdge = [&] (int x, int y) {
        if (x > y) std::swap(x, y);
        if (!indexOfEdge.count({x, y})) {
            indexOfEdge[{x, y}] = counter++;
        }
    };
    auto getEdge = [&] (int x, int y) {
        if (x > y) std::swap(x, y);
        return indexOfEdge.at({x, y});
    };
    for (int i = 0; i < R; i++) {
        addEdge(i, (i+1) % R);
    }
    vector<tuple<int, int, int>> threes;
    for (auto tri : triangles) {
        auto [a,b,c] = tri;
        addEdge(a, b);
        addEdge(b, c);
        addEdge(c, a);
        threes.push_back({
            getEdge(a, b),
            getEdge(b, c),
            getEdge(c, a)
        });
    }
    return threes;
}

// conf ファイルを読み込み、双対グラフの dconf を ofs に出力する
void OutputDuality(string confFile, ostream& ofs) {
    ifstream ifs(confFile);
    if (!ifs) {
        fmt::print("Failed to read {}", confFile);
        return;
    }
    string tmp;
    std::getline(ifs, tmp);
    int N, R;
    ifs >> N >> R;
    set<pair<int, int>> primalEdges;
    for (int i = 0; i < R; i++) {
        int j = (i + 1) % R;
        primalEdges.insert({i, j});
        primalEdges.insert({j, i});
    }
    for (int i = R; i < N; i++) {
        int x, m;
        ifs >> x >> m;
        x--;
        for (int j = 0; j < m; j++) {
            int y;
            ifs >> y;
            y--;
            primalEdges.insert({x, y});
            primalEdges.insert({y, x});
        }
    }
    auto threes = GetDuality(N, R, primalEdges);
    ofs << threes.size() << " " << R << std::endl;
    for (auto [a, b, c] : threes) {
        ofs << a << " " << b << " " << c << std::endl;
    }
}