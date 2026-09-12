#pragma once
#include "coloring.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <filesystem>
using std::cout;
using std::endl;
using std::ofstream;
using std::ifstream;

void GenerateColors(int max_size) {
    std::filesystem::create_directories("color/cubic");
    for (int s = 1; s <= max_size; s++) {
        auto res = Coloring::GetValidColorings(s);
        auto filename = "color/cubic/colors_" + std::to_string(s) + ".txt";
        ofstream ofs(filename);
        spdlog::info("Writing to {}", filename);
        ofs << res.size() << endl;
        for (auto& col : res) {
            ofs << col.StringOf() << endl;
        }
    }
}

vector<Coloring> LoadColorFile(int size) {
    auto filename = "color/cubic/colors_" + std::to_string(size) + ".txt";
    ifstream ifs(filename);
    if (!ifs) {
        spdlog::critical("Error: Failed to open {}", filename);
        throw std::runtime_error("Error opening " + filename);
    }
    spdlog::debug("Reading from {}", filename);
    int len;
    ifs >> len;
    assert(len >= 0);
    vector<Coloring> res;
    for (int i = 0; i < len; i++) {
        string str;
        ifs >> str;
        res.push_back(Coloring(str));
    }
    return res;
}