#pragma once
#include <vector>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <concepts>
#include <spdlog/spdlog.h>
using std::vector;
using std::ifstream;
using std::string;
using std::cerr;
using std::endl;
using std::pair;

template<typename T>
concept Configuration = requires(T a) {
    { a.edge_size } -> std::convertible_to<int>;
    { a.CanColorWith(Coloring(""), vector<bool>()) } -> std::convertible_to<bool>;
};

template <Configuration Conf>
struct RingShape { using Type = void; }; 

// 3 辺彩色をしたい 3 正則グラフ (双対側のグラフ)
class CubicConf {
public:
    const int edge_size; // 辺の個数
    const int ring_size; // リングを通過している辺の個数
protected:
    vector<vector<pair<int, int>>> EtoEE; // 辺の両端について、ほかにその頂点とつながっている辺の番号のペア
    // [0,e) の辺が色付けされているとき、残りの辺を 3 彩色可能か
    bool color_dfs(int e, vector<int> &color_tmp, const vector<bool> &exists) const {
        if (e == edge_size) {
            return true;
        }
        vector<int> isUsed(4);
        if (!exists[e]) {
            return color_dfs(e + 1, color_tmp, exists);
        }
        for (auto [f, g] : EtoEE[e]) {
            assert(exists[f] || exists[g]); // vertex degree must not be 1
            if (exists[f] && exists[g]) {
                if (exists[f]) {
                    if (f < e) {
                        isUsed[color_tmp[f]]++;
                    }
                }
                if (exists[g]) {
                    if (g < e) {
                        isUsed[color_tmp[g]]++;
                    }
                }
            }
            else {
                if (exists[f]) {
                    if (f < e) {
                        for (int d = 1; d <= 3; d++) {
                            if (color_tmp[f] != d) {
                                isUsed[d]++;
                            }
                        }
                    }
                }
                if (exists[g]) {
                    if (g < e) {
                        for (int d = 1; d <= 3; d++) {
                            if (color_tmp[g] != d) {
                                isUsed[d]++;
                            }
                        }
                    }
                }
            }
        }
        for (int c = 1; c <= 3; c++) {
            if (!isUsed[c]) {
                color_tmp[e] = c;
                if (color_dfs(e + 1, color_tmp, exists)) {
                    return true;
                }
            }
        }
        return false;
    }
public:
    // edge_size: 辺の個数
    // ring_size: リングを通過している辺の個数 (0..ring_size が リング上、ring_size.. が内部の辺に対応)
    // VtoE：各頂点について、隣接している辺 3 つからなる vector の vector
    // 例: {{0,7,17},{1,8,18},{2,9,10},...}
    CubicConf(int edge_size, int ring_size, vector<vector<int>> VtoE): edge_size(edge_size), ring_size(ring_size) {
        EtoEE.resize(edge_size);
        for(auto& es : VtoE) {
            assert(es.size() == 3);
            const auto e1 = es[0];
            const auto e2 = es[1];
            const auto e3 = es[2];
            EtoEE[e1].emplace_back(e2, e3);
            EtoEE[e2].emplace_back(e3, e1);
            EtoEE[e3].emplace_back(e1, e2);
        }
    }
    // ifstream から入力を受け取り、CubicConf を返す
    static CubicConf fromFile(ifstream& ifs) {
        int vertexSize, ringSize;
        ifs >> vertexSize >> ringSize;
        assert(vertexSize > 0 && ringSize > 0);
        int edgeSize = (vertexSize * 3 - ringSize) / 2 + ringSize;
        vector<vector<int>> VtoE(vertexSize);
        for (int i = 0; i < vertexSize; i++) {
            for (int j = 0; j < 3; j++) {
                int e;
                ifs >> e;
                assert(0 <= e && e < edgeSize);
                VtoE[i].push_back(e);
            }
        }
        spdlog::info("Vertex size: {}, Edge size: {}, Ring size: {}", vertexSize, edgeSize, ringSize);
        return CubicConf(edgeSize, ringSize, VtoE);
    }
    // colors: リング上の各辺に対して色 [1,2,3] のいずれかを割り当てるような彩色が可能か
    bool CanColorWith(Coloring colors, const vector<bool> &exists, bool isRingIndependent = true) const {
        assert(colors.size() == (unsigned)ring_size);
        auto colorVec = colors.VectorOf();
        colorVec.resize(edge_size);
        if (!isRingIndependent) {
            for (int r = 0; r < ring_size; r++) {
                for (auto [e1, e2] : EtoEE[r]) {
                    if (e1 < r) {
                        if (colorVec[e1] == colorVec[r]) {
                            return false;
                        }
                    }
                    if (e2 < r) {
                        if (colorVec[e2] == colorVec[r]) {
                            return false;
                        }
                    }
                }
            }
        }
        auto res = color_dfs(ring_size, colorVec, exists);
        return res;
    }

    // 縮約する辺の集合 (1/0) を受け取り、それが valid なものか (ring 上の辺を縮約してしまうようなものではないか) を返す
    bool IsContractValid(const vector<int>& exists) {
        auto alive = [&](int e) {
            return (e < ring_size) || (exists[e - ring_size]);
        };
        auto deg = [&](int e, int f, int g) {
            return (alive(e) ? 1 : 0) + (alive(f) ? 1 : 0) + (alive(g) ? 1 : 0);
        };
        for (int e = 0; e < edge_size; e++) {
            for (auto [f, g] : EtoEE[e]) {
                if (deg(e, f, g) == 1) {
                    spdlog::trace("deg({}, {}, {}) = 1", e, f, g);
                    return false;
                }
            }
        }
        return true;
    }

    // valid な縮約方法 (次数 1 が存在しないような縮約方法) を賢く列挙
    vector<vector<bool>> GetGoodContractions(int contSizeMin, int contSizeMax) const {
        vector<vector<bool>> existsList;
        // exists：各辺について、-1: 未設定, 0: 削除する, 1: 残す
        // e を更新した後と仮定し、次数 1 が存在しない条件を exists に適用 
        auto place = [&] (auto&& place, vector<int8_t>& exists, int e) -> bool {
            assert(exists[e] >= 0);
            for (auto [f, g] : EtoEE[e]) {
                spdlog::trace("{} -> {}, {}", e, f, g);
                if (exists[e] == 1) {
                    if (exists[f] == 0) {
                        if (exists[g] == 1) continue;
                        if (exists[g] == 0) return false;
                        exists[g] = 1;
                        if (!place(place, exists, g)) return false;
                    }
                    if (exists[g] == 0) {
                        if (exists[f] == 1) continue;
                        if (exists[f] == 0) return false;
                        exists[f] = 1;
                        if (!place(place, exists, f)) return false;
                    }
                }
                if (exists[e] == 0) {
                    if (exists[f] == 0) {
                        if (exists[g] == 0) continue;
                        if (exists[g] == 1) return false;
                        exists[g] = 0;
                        if (!place(place, exists, g)) return false;
                    }
                    if (exists[f] == 1) {
                        if (exists[g] == 1) continue;
                        if (exists[g] == 0) return false;
                        exists[g] = 1;
                        if (!place(place, exists, g)) return false;
                    }
                    if (exists[g] == 0) {
                        if (exists[f] == 0) continue;
                        if (exists[f] == 1) return false;
                        exists[f] = 0;
                        if (!place(place, exists, f)) return false;
                    }
                    if (exists[g] == 1) {
                        if (exists[f] == 1) continue;
                        if (exists[f] == 0) return false;
                        exists[f] = 1;
                        if (!place(place, exists, f)) return false;
                    }
                }
            }
            return true;
        };
        // 再起関数
        auto recurse = [&](auto&& recurse, vector<int8_t> exists, int e) -> void {
            spdlog::trace("Checking edge {}", e);
            // spdlog::trace("Now: {}", fmt::join(exists, ", "));
            if (e == edge_size) {
                vector<bool> boolExists;
                int contSize = 0;
                for (const auto& elm : exists) {
                    assert(elm >= 0);
                    boolExists.push_back(elm > 0);
                    contSize += (elm == 0) ? 1 : 0;
                }
                if (contSizeMin <= contSize && contSize <= contSizeMax) {
                    spdlog::trace("Obtained {}", fmt::join(exists, ", "));
                    existsList.push_back(boolExists);
                }
                else {
                    spdlog::trace("Skipped {}", fmt::join(exists, ", "));
                }
            }
            else if (exists[e] >= 0) {
                spdlog::trace("Skip {}: {})", e, exists[e]);
                recurse(recurse, exists, e + 1);
            }
            else {
                int deleteCnt = 0;
                for (auto val : exists) if (val == 0) deleteCnt += 1;
                vector<int8_t> exists2 = exists;
                exists[e] = 0;
                exists2[e] = 1;
                spdlog::trace("Color {}: {}", e, 0);
                if (place(place, exists, e)) {
                    if (deleteCnt + 1 > contSizeMax) {
                        spdlog::trace("Prune {} at {})", fmt::join(exists, ", "), e);
                    }
                    else {
                        recurse(recurse, exists, e+1);
                    }
                }
                spdlog::trace("Color {}: {}", e, 1);
                if (place(place, exists2, e)) {
                    if (deleteCnt > contSizeMax) {
                        spdlog::trace("Prune {} at {})", fmt::join(exists2, ", "), e);
                    }
                    else {
                        recurse(recurse, exists2, e+1);
                    }
                }
            }
        };
        vector<int8_t> exists(edge_size, -1);
        for (int i = 0; i < ring_size; i++) exists[i] = 1;
        recurse(recurse, exists, ring_size);
        return existsList;
    }

    // Coloring の情報を受け取り、各 Coloring に対して内部彩色が可能かを返す
    vector<bool> CheckColorability(const vector<Coloring>& ringColorings, const vector<int>& contractEdges = vector<int>(), bool isRingIndependent = true) const {
        int feasibleCount = 0;
        vector<bool> exists(edge_size, true);
        for (auto contract : contractEdges) {
            assert(contract >= ring_size && contract < edge_size);
            exists[contract] = false;
        }
        vector<bool> res;
        for (auto& colors : ringColorings) {
            auto feasible = CanColorWith(colors, exists, isRingIndependent);
            res.push_back(feasible);
            if (feasible) {
                feasibleCount += 1;
                spdlog::trace("{}: OK", colors.StringOf());
            }
            else {
                spdlog::trace("{}: NG", colors.StringOf());
            }
        }
        spdlog::debug("Coloring result: {} / {}", feasibleCount, ringColorings.size());
        if (feasibleCount == 0) {
            spdlog::debug("This contraction may be broken...");
            for (int i = 0; i < (int)res.size(); i++) res[i] = true; 
        } 
        return res;
    }
};

template <>
struct RingShape<CubicConf> { using Type = int; };

class AnnularCubicConf : public CubicConf {
public:
    const int left_ring_size;
    const int right_ring_size;
    AnnularCubicConf(int edge_size, int leftSize, int rightSize, vector<vector<int>> VtoE)
        :CubicConf(edge_size, leftSize + rightSize, VtoE), left_ring_size(leftSize), right_ring_size(rightSize) {}
    pair<size_t, size_t> annularRing() const {
        return {left_ring_size, right_ring_size};
    }
    static AnnularCubicConf fromFile(ifstream& ifs) {
        int vertexSize, leftSize, rightSize;
        ifs >> vertexSize >> leftSize >> rightSize;
        assert(vertexSize > 0 && leftSize > 0 && rightSize > 0);
        int ringSize = leftSize + rightSize;
        int edgeSize = (vertexSize * 3 - ringSize) / 2 + ringSize;
        vector<vector<int>> VtoE(vertexSize);
        for (int i = 0; i < vertexSize; i++) {
            for (int j = 0; j < 3; j++) {
                int e;
                ifs >> e;
                assert(0 <= e && e < edgeSize);
                VtoE[i].push_back(e);
            }
        }
        spdlog::info("Vertex size: {}, Edge size: {}, Ring size: {} ({} + {})", vertexSize, edgeSize, ringSize, leftSize, rightSize);
        return AnnularCubicConf(edgeSize, leftSize, rightSize, VtoE);
    }
};

template <>
struct RingShape<AnnularCubicConf> { using Type = std::pair<int, int>; };
