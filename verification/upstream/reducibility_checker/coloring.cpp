#include <iostream>
#include "coloring.hpp"
using std::cerr;
using std::endl;

vector<string> Coloring::FourDFS(const vector<string> &fours, int n) {
    if ((int)fours[0].size() >= n) {
        return fours;
    }
    vector<string> mores;
    for (auto& four : fours) {
        for (char c = '1'; c <= '4'; c++) {
            if (c != four.back()) {
                if (n == (int)four.size() + 1 && c == four[0]) {
                    continue;
                }
                mores.push_back(four + c);
            }
        }
    }
    return FourDFS(mores, n);
}

vector<Coloring> Coloring::GetEquivalents() const {
    string other = str;
    for (auto& c : other) {
        switch(c) {
            case '2': c = 3; break;
            case '3': c = 2; break;
        }
    }
    return {*this, Coloring(other)};
}

unordered_set<Coloring> Coloring::GetValidColorings(int n) {
    assert(n >= 1);
    if (n == 1) {
        return {Coloring("1")};
    }
    if (n == 2) {
        return {Coloring("11")};
    }
    if (n == 3) {
        return {Coloring("123")};
    }
    auto fours = FourDFS({"121", "123"}, n); // 4 頂点彩色を得る
    unordered_set<Coloring> res;
    for (auto& four : fours) {
        assert((int)four.size() == n);
        string three;
        for (int i = 0; i < n; i++) {
            int x = four[i] - '1';
            int y = four[(i + 1) % n] - '1';
            three += '0' + (x ^ y);
        }
        // 辞書順最小となるように色を交換
        char flags[3] = {};
        char a = '1';
        for (auto& c : three) {
            assert('1' <= c && c <= '3');
            if (flags[c - '1']) {}
            else {
                flags[c - '1'] = a++;
            }
            c = flags[c - '1'];
        }
        res.insert(Coloring(three));
    }
    return res;
}

vector<Coloring> Coloring::GetKempeChanges(const string& kempe, int fix) const {
    vector<Coloring> res;
    for (unsigned long long bits = 0ull; bits < (1ull << (kempe.size() / 2 - 1)); bits++) {
        string newStr = str;
        int index = 0;
        for (auto& c : newStr) {
            if (c == fix + '0') continue;
            int k = int(kempe[index] - 'a') - 1;
            index++;
            if (k < 0) continue;
            if (bits & (1ll << k)) {
                c = '0' + ((c - '0') ^ fix);
            }
        }
        res.push_back(Coloring(newStr));
        res.back().lexicalMin();
    }
    return res;
}