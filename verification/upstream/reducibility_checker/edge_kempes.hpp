#pragma once
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <spdlog/spdlog.h>
using std::vector;
using std::unordered_set;
using std::unordered_map;
using std::string;
using std::reverse;
using std::cout;
using std::endl;

// s と同値な Kempe chain のうち、辞書順最小なものに変更する
void reassign(string& s) {
    unordered_map<char, int> nums;
    char a = 'a';
    for (auto& c : s) {
        if (nums.count(c)) {}
        else {
            nums[c] = a++;
        }
        c = nums[c];
    }
}

// 長さ 2n の valid な括弧列を返す (n > 0)
// 例：n = 2 の場合 {"(())", "()()"} 
unordered_set<string> GetValidParens(int n) {
    assert(n >= 1);
    if (n == 1) {
        return {"()"};
    }
    unordered_set<string> res;
    for (int i = 1; i < n; i++) {
        auto ls = GetValidParens(i);
        auto rs = GetValidParens(n - i);
        for (auto& l : ls) {
            for (auto& r : rs) {
                res.insert(l + r);
            }
        }
    }
    auto ms = GetValidParens(n - 1);
    for (auto& s : ms) {
        res.insert("(" + s + ")");
    }
    return res;
}

// 長さ 2n のリング上に存在する n 組の Kempe chain の両端を返す
// 例：n = 2 の場合 {"aabb", "abba"}
unordered_set<string> GetPlanarKempes(int n) {
    auto parens = GetValidParens(n);
    unordered_set<string> res;
    for (auto s : parens) {
        char a = 'a';
        vector<char> stack;
        for (auto& c : s) {
            if (c == '(') {
                stack.push_back(a++);
                c = stack.back();
            }
            else {
                c = stack.back();
                stack.pop_back();
            }
        }
        res.insert(s);
    }
    return res;
}

// GetPlanarKempes で得られた長さ 2(n-1) の Kempe pairs の任意の区間を反転させる 
// 例：n = 3 の場合、例えば "aabb" から "ccaabb", "acabcb", "aabcbc" などが得うる
unordered_set<string> GetFlippedKempes(int n) {
    if (n == 1) {
        return {"aa"};
    }
    auto kempes = GetPlanarKempes(n - 1);
    char c = 'a' + n - 1;
    unordered_set<string> res;
    for (auto& s : kempes) {
        for (auto i = 0u; i < s.size(); i++) {
            for (auto j = i; j <= s.size(); j++) {
                auto rev = s.substr(i, j - i);
                reverse(rev.begin(), rev.end());
                string t = s.substr(0, i) + c + rev + c + s.substr(j, -1);
                res.insert(t);
            }
        }
    }
    return res;
}

// GetFlippedKempes の同値なものを除去し、射影平面上の Kempe chain を返す
// 同値なものの例：cababc、abcbca、bacacb など
unordered_set<string> GetProjectiveKempes(int n) {
    auto flipped = GetFlippedKempes(n);
    unordered_set<string> res;
    for (auto s : flipped) {
        reassign(s);
        res.insert(s);
    }
    return res;
}

// GetPlanarKempes で得られた長さ 2(n-1) の Kempe pairs に一つ (交差しうる) Kempe pair を追加する (Kempe pair 内は反転せずそのまま)
// 例：n = 3 の場合、例えば "aabb" から "ccaabb", "acbacb", "aabcbc" などが得うる
unordered_set<string> GetUnflippedKempes(int n) {
    if (n == 1) {
        return {"aa"};
    }
    auto kempes = GetPlanarKempes(n - 1);
    char c = 'a' + n - 1;
    unordered_set<string> res;
    for (auto& s : kempes) {
        for (auto i = 0u; i < s.size(); i++) {
            for (auto j = i; j <= s.size(); j++) {
                auto bop = s.substr(i, j - i);
                string t = s.substr(0, i) + c + bop + c + s.substr(j, -1);
                res.insert(t);
            }
        }
    }
    return res;
}

// GetUnflippedKempes の同値なものを除去し、Apex グラフ上の Kempe chain を返す
unordered_set<string> GetApexKempes(int n) {
    auto flipped = GetUnflippedKempes(n);
    unordered_set<string> res;
    for (auto s : flipped) {
        reassign(s);
        res.insert(s);
    }
    return res;
}

// GetPlanarKempes で得られた長さ 2(n-1) の Kempe pairs の任意の区間を「回転」させる 
unordered_set<string> GetRotatedKempes(int n) {
    if (n == 1) {
        return {"aa"};
    }
    auto kempes = GetPlanarKempes(n - 1);
    char c = 'a' + n - 1;
    unordered_set<string> res;
    for (auto& s : kempes) {
        for (auto i = 0u; i < s.size(); i++) {
            for (auto j = i; j <= s.size(); j++) {
                auto bop = s.substr(i, j - i);
                for (int k = 0; k < std::max((int)bop.size(), 1); k++) {
                    string t = s.substr(0, i) + c + bop.substr(k, -1) + bop.substr(0, k) + c + s.substr(j, -1);
                    //spdlog::info("{}, {}, {}, {}", s, i, j, t);
                    res.insert(t);
                }
            }
        }
    }
    return res;
}

unordered_set<string> GetToroidalKempes(int n) {
    auto rotated = GetRotatedKempes(n);
    unordered_set<string> res;
    for (auto s : rotated) {
        reassign(s);
        res.insert(s);
    }
    return res;
}

unordered_set<string> GetAnnularKempes(int l, int r) {
    unordered_set<string> res;
    for (string s : GetPlanarKempes((l + r) / 2)) {
        string r1 = s.substr(0, l);
        string r2 = s.substr(l, r);
        std::reverse(r2.begin(), r2.end());
        string t = r1 + r2;
        reassign(t);
        res.insert(t);
    }
    for (int i = 0; i < l; i++) {
        for (int j = 0; j < r; j++) {
            int m = (l + r) / 2 - 1;
            auto kempes = m ? GetPlanarKempes(m) : unordered_set<string>{""};
            char c = 'a' + m;
            for (auto& s : kempes) {
                string r1 = s.substr(i, j);
                string r2 = s.substr(i + j, r - j - 1);
                std::reverse(r1.begin(), r1.end());
                std::reverse(r2.begin(), r2.end());
                string t = s.substr(0, i) + c + s.substr(r + i - 1, l - i - 1) + r1 + c + r2;
                reassign(t);
                if (l == 3 && r == 3 && !t.compare("abccba")) {
                    std::cerr << s <<" "<< i <<" "<< j<<" " << l <<" "<< r << endl;
                    assert(false);
                }
                res.insert(t);
            }
        }
    }
    return res;
}