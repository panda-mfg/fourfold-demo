#pragma once
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <utility>
#include <cassert>

using std::vector;
using std::unordered_set;
using std::unordered_map;
using std::string;
using std::pair;

class Coloring {
    string str;
    static vector<string> FourDFS(const vector<string> &fours, int n);
    void lexicalMin() {
        for (char &c : str){
            if (c == '2') return;
            if (c == '3') {
                for (char &d : str) {
                    if (d == '2') d = '3';
                    else if (d == '3') d = '2';
                }
                return;
            }
        }
    }
public:
    Coloring(string str): str(str) {}
    bool operator==(const Coloring& o) const {
        return str == o.str;
    }
    const string& StringOf() const {
        return str;
    }
    vector<int> VectorOf() const {
        vector<int> res;
        for (auto& c : str) {
            res.push_back(c - '0');
        }
        return res;
    }
    size_t size() const {
        return str.size();
    }
    int sizeWithout(int c) const {
        int res = 0;
        for (auto d : str) 
            res += (d != char(c + '0') ? 1 : 0);
        return res;
    }
    pair<int, int> sizeWithout(int c, int l) const {
        int lres = 0;
        int rres = 0;
        for (int i = 0; i < (int)str.size(); i++) {
            if (str[i] != char(c + '0')) {
                if (i < l) lres++;
                else rres++;
            }
        }
        return {lres, rres};
    }
    // 123 -> {123,132} を返す
    vector<Coloring> GetEquivalents() const;
    // 大きさ n のリングの (parity が valid な) 3 彩色を得る
    static unordered_set<Coloring> GetValidColorings(int n);
    // 色 fix を固定し、それ以外の 2 色を kempe によって change した結果得られる全ての Coloring を返す
    vector<Coloring> GetKempeChanges(const string& kempe, int fix) const;
    // 辞書順最小とは限らない string を受け取り、辞書順最小の Coloring を返す
    static Coloring GetLexicalMin(const string& str) {
        unordered_map<char, char> m;
        char a = '1';
        string res;
        for (auto& c : str) {
            if (m.count(c)) {}
            else {
                m[c] = a++;
            }
            res.push_back(m.at(c));
        }
        return Coloring(res);
    }
};

namespace std {
    template <>
    struct hash<Coloring> {
        std::size_t operator()(const Coloring& c) const {
            return std::hash<string>()(c.StringOf());
        }
    };
}