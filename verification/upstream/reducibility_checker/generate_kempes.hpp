#pragma once
#include "edge_kempes.hpp"
#include <fstream>
#include <spdlog/spdlog.h>
#include <filesystem>
using std::ifstream;
using std::ofstream;

enum KempeType {
    Planar, Projective, Apex, Toroidal
};

void GenerateKempes(int max_size) {
    std::filesystem::create_directories("kempes/plan");
    std::filesystem::create_directories("kempes/proj");
    std::filesystem::create_directories("kempes/apex");
    std::filesystem::create_directories("kempes/tori");
    std::filesystem::create_directories("kempes/annu");
    for (int s = 1; s <= max_size; s++) {
        {
            auto res = GetPlanarKempes(s);
            auto filename = "kempes/plan/kempes_" + std::to_string(s) + ".txt";
            ofstream ofs(filename);
            spdlog::info("Writing to {}", filename);
            ofs << res.size() << endl;
            for (auto& s : res) {
                ofs << s << endl;
            }
        }
        {
            auto res = GetProjectiveKempes(s);
            auto filename = "kempes/proj/kempes_" + std::to_string(s) + ".txt";
            ofstream ofs(filename);
            spdlog::info("Writing to {}", filename);
            ofs << res.size() << endl;
            for (auto& s : res) {
                ofs << s << endl;
            }
        }
        {
            auto res = GetApexKempes(s);
            auto filename = "kempes/apex/kempes_" + std::to_string(s) + ".txt";
            ofstream ofs(filename);
            spdlog::info("Writing to {}", filename);
            ofs << res.size() << endl;
            for (auto& s : res) {
                ofs << s << endl;
            }
        }
        {
            auto res = GetToroidalKempes(s);
            auto filename = "kempes/tori/kempes_" + std::to_string(s) + ".txt";
            ofstream ofs(filename);
            spdlog::info("Writing to {}", filename);
            ofs << res.size() << endl;
            for (auto& s : res) {
                ofs << s << endl;
            }
        }
        {
            for (int l = 1; l < s * 2; l++) {
                int r = s * 2 - l;
                auto res = GetAnnularKempes(l, r);
                auto filename = "kempes/annu/kempes_" + std::to_string(l) + "_" + std::to_string(r) + ".txt";
                ofstream ofs(filename);
                spdlog::info("Writing to {}", filename);
                ofs << res.size() << endl;
                for (auto& s : res) {
                    ofs << s << endl;
                }
            }
        }
    }
}

vector<string> LoadKempeFile(int size, KempeType type) {
    auto folderName = (type == Planar) ? "plan" : (type == Projective) ? "proj" : (type == Apex) ? "apex" : "tori";
    auto filename = string("kempes/") + folderName + string("/kempes_") + std::to_string(size) + ".txt";
    ifstream ifs(filename);
    if (!ifs) {
        spdlog::critical("Error: Failed to open {}", filename);
        throw std::runtime_error("Failed to open " + filename);
    }
    spdlog::debug("Reading from {}", filename);
    int len;
    ifs >> len;
    assert(len >= 0);
    vector<string> res;
    for (int i = 0; i < len; i++) {
        string str;
        ifs >> str;
        res.push_back(str);
    }
    return res;
}

vector<string> LoadAnnularKempeFile(int leftSize, int rightSize) {
    auto folderName = "kempes/annu";
    auto filename = folderName + string("/kempes_") + std::to_string(leftSize) + "_" + std::to_string(rightSize) + ".txt";
    ifstream ifs(filename);
    if (!ifs) {
        spdlog::critical("Error: Failed to open {}", filename);
        throw std::runtime_error("Failed to open " + filename);
    }
    spdlog::debug("Reading from {}", filename);
    int len;
    ifs >> len;
    assert(len >= 0);
    vector<string> res;
    for (int i = 0; i < len; i++) {
        string str;
        ifs >> str;
        res.push_back(str);
    }
    return res;
}