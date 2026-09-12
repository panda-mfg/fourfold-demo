#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <spdlog/spdlog.h>
using std::ofstream;
using std::ifstream;
using std::vector;
using std::string;

// 各 coloring が feasible かどうかの 01 列をテキストとして出力
void WriteFeasibles(vector<bool> isFeasible, string fileName) {
    ofstream ofs(fileName);
    if (!ofs) {
        spdlog::critical("Error: Failed to open {}", fileName);
        throw std::runtime_error("Error opening " + fileName);
    }
    spdlog::debug("Writing feasibles to {}", fileName);
    for (auto b : isFeasible) {
        ofs << (b ? '1' : '0');
    }
    ofs << std::endl;
}

// feasible 列を外から読み込む
vector<bool> LoadFeasibles(string fileName) {
    ifstream ifs(fileName);
    if (!ifs) {
        spdlog::critical("Error: Failed to open {}", fileName);
        throw std::runtime_error("Error opening " + fileName);
    }
    spdlog::debug("Reading feasibles from {}", fileName);
    string str;
    ifs >> str;
    vector<bool> res;
    for (auto c : str) {
        if (!(c == '0' || c == '1')) {
            spdlog::critical("Error: {} contains character \'{}\' ({})", fileName, c, (int)c);
            throw std::runtime_error("Error reading " + fileName);
        }
        res.push_back(c == '1');
    }
    return res;
}