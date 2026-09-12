#pragma once
#include <string>
#include <algorithm>
#include <map>
#include <optional>
#include <spdlog/spdlog.h>
#include "generate_kempes.hpp"
#include "generate_colors.hpp"
#include "cubic_conf.hpp"
#include "feasibles.hpp"

using std::string;

// 一回分の feasibility update を行い、infeasible -> feasible にできた Coloring の個数を返す
template <Configuration Conf>
int OneReduction(int colorNum, const vector<Coloring>& normalColorings, vector<bool>& feasible,
    typename RingShape<Conf>::Type originalRingShape, 
    const std::map<typename RingShape<Conf>::Type, vector<string>>& allKempes, vector<vector<int>>& kempeIndexes, 
    const unordered_map<Coloring, int>& coloringRev) {
    using RingType = typename RingShape<Conf>::Type;
    auto& newFeasible = feasible;
    const auto isFeasible = feasible;
    int updateCount = 0;
    for (int i = 0; i < colorNum; i++) {
        auto& colors = normalColorings[i];
        auto varFeasible = isFeasible[i];
        if (!varFeasible) {
            bool someColorWorks = false;
            spdlog::debug("Checking for Coloring: {}", colors.StringOf());
            for (int fix = 1; fix <= 3; fix++) {
                spdlog::trace("Checking for fix = {}", fix);
                RingType withoutSize;
                if constexpr (std::same_as<RingType, int>) {
                    withoutSize = colors.sizeWithout(fix);
                    if (withoutSize == 0) {
                        spdlog::trace("Color does not exist in ring");
                        continue;
                    }
                }
                else if constexpr (std::same_as<RingType, pair<int, int>>) {
                    withoutSize = colors.sizeWithout(fix, originalRingShape.first);
                    if (withoutSize.first == 0 && withoutSize.second == 0) {
                        spdlog::trace("Color does not exist in ring");
                        continue;
                    }
                }
                else {
                    static_assert(!std::same_as<RingType, void>);
                }
                auto& kempes = allKempes.at(withoutSize);
                bool everyKempeWorks = true;
                auto& kempeIndex = kempeIndexes[i][fix - 1];
                spdlog::trace("Checking {} kempe chains from {}", kempes.size(), kempeIndex);
                for (;kempeIndex < (int)kempes.size(); kempeIndex++) {
                    auto& kempe = kempes[kempeIndex];
                    auto kempeChanges = colors.GetKempeChanges(kempe, fix);
                    bool changable = false;
                    spdlog::trace("[{}/{}] {}", kempeIndex, kempes.size(), kempe);
                    int changedIndex = 0;
                    for (auto& changedColor : kempeChanges) {
                        spdlog::trace("[[{}/{}]] {}", changedIndex, kempeChanges.size(), changedColor.StringOf());
                        if (!coloringRev.count(changedColor)) {
                            spdlog::trace("{} does not exist in rev", changedColor.StringOf());
                        }
                        // 同時更新をする (iteration 回数が少なくなる？)
                        if (feasible[coloringRev.at(changedColor)]) {
                            changable = true;
                            break;
                        }
                        changedIndex++;
                    }
                    if (!changable) {
                        spdlog::debug("Failed on [[{}/{}]] {}", changedIndex, kempeChanges.size(), kempe);
                        everyKempeWorks = false;
                        break;
                    }
                }
                if (everyKempeWorks) {
                    spdlog::debug("Every kempe chain works!");
                    someColorWorks = true;
                    break;
                }
            }
            newFeasible[i] = someColorWorks;
            updateCount += someColorWorks ? 1 : 0;
        }
    }
    for (int i = 0; i < colorNum; i++) { 
        auto& colors = normalColorings[i];
        if (newFeasible[i]) {
            spdlog::trace("{}: OK", colors.StringOf());
        }
        else {
            spdlog::trace("{}: NG", colors.StringOf());
        }
    }
    return updateCount;
}

template <Configuration Conf>
vector<bool> CheckDReducibility(Conf& conf, KempeType type, bool skipDReducibility) {
    vector<vector<Coloring>> allColorings;
    for (int s = 1; s <= conf.ring_size; s++) {
        auto colorings = LoadColorFile(s);
        allColorings.push_back(colorings);
    }
    using RingType = typename RingShape<Conf>::Type;
    std::map<RingType, vector<string>> allKempes;
    RingType originalRingShape;
    if constexpr (std::same_as<RingType, pair<int, int>>) {
        originalRingShape = conf.annularRing();
        for (int l = 0; l <= conf.left_ring_size; l++) {
            for (int r = 0; r <= conf.right_ring_size; r++) {
                if ((l + r) % 2 == 1) continue;
                if (l + r == 0) continue;
                if (l == 0) {
                    auto kempes = LoadKempeFile(r / 2, Planar);
                    allKempes.insert({{l, r}, kempes});
                }
                else if (r == 0) {
                    auto kempes = LoadKempeFile(l / 2, Planar);
                    allKempes.insert({{l, r}, kempes});
                }
                else {
                    auto kempes = LoadAnnularKempeFile(l, r);
                    allKempes.insert({{l, r}, kempes});
                }
            }
        }
    }
    else if constexpr (std::same_as<RingType, int>) {
        originalRingShape = conf.ring_size;
        for (int s = 1; s <= conf.ring_size / 2; s++) {
            auto kempes = LoadKempeFile(s, type);
            allKempes.insert({s * 2, kempes});
        }
    }
    else {
        static_assert(!std::same_as<RingType, void>);
    }
    auto& normalColorings = allColorings[conf.ring_size - 1];
    auto isFeasible = conf.CheckColorability(normalColorings, {}, false);
    if (skipDReducibility) {
        spdlog::info("Skipped D-reducibility check");
        return isFeasible;
    }
    int feasibleCount = std::count(isFeasible.begin(), isFeasible.end(), true);
    int colorNum = normalColorings.size();
    unordered_map<Coloring, int> coloringRev;
    for (int i = 0; i < colorNum; i++) {
        coloringRev[normalColorings[i]] = i;
    }
    vector<vector<int>> kempeIndexes(colorNum, vector<int>(3));
    int iterationCount = 0;
    spdlog::info("Started D-reducibility check");
    while (feasibleCount != colorNum) {
        spdlog::info("#{}: Feasible / Total: {} / {}", iterationCount + 1, feasibleCount, colorNum);
        int updateCount = OneReduction<Conf>(colorNum, normalColorings, isFeasible, originalRingShape, allKempes, kempeIndexes, coloringRev);
        feasibleCount += updateCount;
        if (updateCount == 0) {
            break;
        }
        iterationCount++;
    }
    spdlog::info("#{}: Feasible / Total: {} / {}", iterationCount + 1, feasibleCount, colorNum);
    if (feasibleCount == colorNum) {
        spdlog::info("Graph is D-reducible!");
    }
    else {
        spdlog::info("Graph is not D-reducible.");
    }
    return isFeasible;
}

// C-reducible に成功したあと、プログラムをどのように停止させるか
enum HaltType {
    HaltImmediately, // すぐに停止
    HaltAfterSameSize, // 同じ大きさの contraction をすべて試したのち停止
    NoHalt, // 停止しない
};

void CheckCReducibility(CubicConf& conf, const vector<bool> &feasible, HaltType haltType, int minCont, int maxCont) {
    auto colorings = LoadColorFile(conf.ring_size);
    int colorNum = colorings.size();
    spdlog::info("Started C-reducibility check");
    bool isCReducible = false;

    auto existsList = conf.GetGoodContractions(minCont, maxCont);
    vector<pair<int, vector<bool>>> existsCount(existsList.size());
    std::transform(existsList.begin(), existsList.end(), existsCount.begin(), [](const vector<bool>& v) {
        return std::make_pair(std::count(v.begin(), v.end(), false), v);
    });
    std::sort(existsCount.begin(), existsCount.end(), [](auto& v1, auto& v2) {return v1.first < v2.first;});
    std::transform(existsCount.begin(), existsCount.end(), existsList.begin(), [](const auto& v) {return v.second;});
    
    spdlog::info("Trying {} possible contractions", existsList.size());
    int contCount = 0;
    int maxContSize = 0;
    int lastContSize = 0;
    for (auto &[contSize, exists] : existsCount) {
        if (isCReducible && contSize > lastContSize && haltType == HaltAfterSameSize) {
            break;
        }
        vector<int> contractEdges;
        for (int i = 0; i < (int)exists.size(); i++) {
            if (!exists[i]) {
                contractEdges.push_back(i);
            }
        }
        if (contractEdges.empty()) continue;
        if (maxContSize < contSize) {
            maxContSize = contSize;
            spdlog::info("[{}/{}] Starting contraction of size {}", contCount, existsList.size(), contSize);
        }
        spdlog::debug("[{}/{}] Contracting: {}", contCount, existsList.size(), fmt::join(contractEdges, ", "));
        auto contFeasible = conf.CheckColorability(colorings, contractEdges);
        bool badColoringExists = false;
        for (int i = 0; i < colorNum; i++) {
            if (contFeasible[i]) {
                spdlog::trace("[{}/{}] {} -> {}", i, colorNum, colorings[i].StringOf(), feasible[i]);
                if(!feasible[i]) {
                    badColoringExists = true;
                    break;
                }
            }
        }
        if (badColoringExists) {
            spdlog::debug("Bad color exists");
        }
        else {
            spdlog::info("All colors passed! Contracted: {}", fmt::join(contractEdges, ", "));
            isCReducible = true;
            if (haltType == HaltImmediately) {
                break;
            }
        }
        lastContSize = contSize;
        contCount++;
    }
    if (isCReducible) {
        spdlog::info("Graph is C-reducible!");
    }
    else {
        spdlog::info("Graph is not C-reducible.");
    }
}

vector<bool> RotatedFeasibles(vector<bool> feasible, int ring_size) {
    auto colorings = LoadColorFile(ring_size);
    int colorNum = colorings.size();
    unordered_map<Coloring, int> coloringRev;
    for (int i = 0; i < colorNum; i++) {
        coloringRev[colorings[i]] = i;
    }
    vector<bool> res(feasible.size());
    for (int i = 0; i < colorNum; i++) {
        if (feasible[i]) {
            string tmp = colorings[i].StringOf();
            for (int j = 0; j < ring_size; j++) {
                auto color = Coloring::GetLexicalMin(tmp);
                res[coloringRev.at(color)] = true;
                tmp = tmp.substr(1) + tmp[0];
            }
        }
    }
    return res;
}

void CheckCReducibilitySingleCase(CubicConf& conf, const vector<bool> &feasible, const vector<int> &edgeSet) {
    spdlog::info("Started C-reducibility check for edge set = [{}]", fmt::join(edgeSet, ", "));
    auto colorings = LoadColorFile(conf.ring_size);
    int colorNum = colorings.size();
    auto contFeasible = conf.CheckColorability(colorings, edgeSet);
    bool badColoringExists = false;
    for (int i = 0; i < colorNum; i++) {
        if (contFeasible[i]) {
            spdlog::trace("[{}/{}] {} -> {}", i, colorNum, colorings[i].StringOf(), feasible[i]);
            if(!feasible[i]) {
                badColoringExists = true;
                break;
            }
        }
    }
    if (badColoringExists) {
        spdlog::info("Graph is not C-reducible for this edge set.");
    }
    else {
        spdlog::info("Graph is C-reducible!");
    }
}

template <Configuration Conf>
void EvaluateConf(string confFile, KempeType type, HaltType haltType, int minContOp, int maxContOp, string feasibleFile, bool readFromFeasible, bool writeToFeasible, bool rotateColoringOfFeasible, bool outputWithoutDReducibleCheck, bool hasEdgeSet, const vector<int> &edgeSet, bool isAnnular) {
    ifstream ifs(confFile);
    if (!ifs) {
        spdlog::error("Failed to read {}", confFile);
        return;
    }
    if (isAnnular) {
        spdlog::info("Kempe type: Annular");
    }
    else {
        switch (type) {
            case Planar: 
                spdlog::info("Kempe type: Planar");
                break;
            case Apex: 
                spdlog::info("Kempe type: Apex");
                break;
            case Projective: 
                spdlog::info("Kempe type: Projective");
                break;
            case Toroidal: 
                spdlog::info("Kempe type: Toroidal");
                break;
        }
    }
    if (hasEdgeSet) {
        spdlog::info("Checking for edge set: [{}]", fmt::join(edgeSet, ", "));
    }
    Conf conf = Conf::fromFile(ifs);
    // std::optional<pair<size_t, size_t>> annularRing = isAnnular ? std::make_optional(conf.annular_ring) : std::nullopt; 
    auto feasible = readFromFeasible ? LoadFeasibles(feasibleFile) : CheckDReducibility(conf, type, outputWithoutDReducibleCheck);
    if (outputWithoutDReducibleCheck) {
        WriteFeasibles(feasible, feasibleFile);
        return;
    }
    if (writeToFeasible) {
        if (rotateColoringOfFeasible) {
            auto rotatedFeasible = RotatedFeasibles(feasible, conf.ring_size);
            WriteFeasibles(rotatedFeasible, feasibleFile);
        }
        else {
            WriteFeasibles(feasible, feasibleFile);
        }
    }
    for (auto f : feasible) {
        if (!f) {
            int minCont = minContOp;
            int maxCont = maxContOp <= 0 ? conf.edge_size : maxContOp;
            if (hasEdgeSet) {
                CheckCReducibilitySingleCase(conf, feasible, edgeSet);
            }
            else {
                CheckCReducibility(conf, feasible, haltType, minCont, maxCont);
            }
            break;
        }
    }
}