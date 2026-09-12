#include "../src/cartwheel.hpp"
#include "../src/configuration.hpp"
#include "../src/pseudo_configuration.hpp"
#include "../src/rule.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class CartWheelFiles : public ::testing::Test {
  protected:
    std::string cartwheelfile1 = "wheel1.cartwheel";
    std::string cartwheelfile2 = "wheel2.cartwheel";
    std::string cartwheelfile3 = "wheel3.cartwheel";

    void SetUp() override {
        std::ofstream out1(cartwheelfile1);
        out1 << "8 1\n";
        out1 << "1 7 7 2 3 4 5 6 7 8\n";
        out1 << "2 5 5 3 1 8 -1\n";
        out1 << "3 5 5 4 1 2 -1\n";
        out1 << "4 6 6 5 1 3 -1\n";
        out1 << "5 5 5 6 1 4 -1\n";
        out1 << "6 5 5 7 1 5 -1\n";
        out1 << "7 5 5 8 1 6 -1\n";
        out1 << "8 9 9 2 1 7 -1\n";
        out1.close();
        std::ofstream out2(cartwheelfile2);
        out2 << "18 1\n";
        out2 << "1 7 7 2 3 4 5 6 7 8\n";
        out2 << "2 5 5 1 8 9 10 3\n";
        out2 << "3 7 7 1 2 10 11 12 13 4\n";
        out2 << "4 5 5 1 3 13 14 5\n";
        out2 << "5 5 5 1 4 14 15 6\n";
        out2 << "6 9 9 16 7 1 5 15 -1\n";
        out2 << "7 5 5 1 6 16 17 8\n";
        out2 << "8 6 6 2 1 7 17 18 9\n";
        out2 << "9 5 9 10 2 8 18 -1\n";
        out2 << "10 5 9 11 3 2 9 -1\n";
        out2 << "11 5 9 12 3 10 -1\n";
        out2 << "12 5 9 13 3 11 -1\n";
        out2 << "13 5 9 14 4 3 12 -1\n";
        out2 << "14 5 9 15 5 4 13 -1\n";
        out2 << "15 5 9 6 5 14 -1\n";
        out2 << "16 5 9 17 7 6 -1\n";
        out2 << "17 5 9 18 8 7 16 -1\n";
        out2 << "18 5 9 9 8 17 -1\n";
        out2.close();
        std::ofstream out3(cartwheelfile3);
        out3 << "20 1\n";
        out3 << "1 7 7 2 3 4 5 6 7 8\n";
        out3 << "2 6 6 3 1 8 9 10 11\n";
        out3 << "3 6 6 4 1 2 11 12 13\n";
        out3 << "4 5 5 5 1 3 13 14\n";
        out3 << "5 5 5 6 1 4 14 15\n";
        out3 << "6 7 7 7 1 5 15 16 17 18\n";
        out3 << "7 5 5 8 1 6 18 19\n";
        out3 << "8 6 6 2 1 7 19 20 9\n";
        out3 << "9 5 9 10 2 8 20 -1\n";
        out3 << "10 5 9 11 2 9 -1\n";
        out3 << "11 5 9 12 3 2 10 -1\n";
        out3 << "12 5 9 13 3 11 -1\n";
        out3 << "13 5 9 14 4 3 12 -1\n";
        out3 << "14 5 9 15 5 4 13 -1\n";
        out3 << "15 5 9 16 6 5 14 -1\n";
        out3 << "16 5 9 17 6 15 -1\n";
        out3 << "17 5 9 18 6 16 -1\n";
        out3 << "18 5 9 19 7 6 17 -1\n";
        out3 << "19 5 9 20 8 7 18 -1\n";
        out3 << "20 5 9 9 8 19 -1\n";
        out3.close();
    }

    void TearDown() override {
        std::filesystem::remove(cartwheelfile1);
        std::filesystem::remove(cartwheelfile2);
        std::filesystem::remove(cartwheelfile3);
    }
};

class ConfFiles : public ::testing::Test {
  protected:
    std::string conffile1 = "conf1.conf";
    std::string conffile2 = "conf2.conf";

    void SetUp() override {
        std::ofstream out1(conffile1);
        out1 << "\n";
        out1 << "17 10\n";
        out1 << "11 5 1 12 17 9 10\n";
        out1 << "12 5 1 2 13 17 11\n";
        out1 << "13 6 2 14 16 7 17 12\n";
        out1 << "14 5 2 3 15 16 13\n";
        out1 << "15 5 3 4 5 16 14\n";
        out1 << "16 6 5 6 7 13 14 15\n";
        out1 << "17 6 7 8 9 11 12 13\n";
        out1.close();
        std::ofstream out2(conffile2);
        out2 << "\n";
        out2 << "11 7\n";
        out2 << "8 5 1 2 9 11 7\n";
        out2 << "9 6 2 3 4 10 11 8\n";
        out2 << "10 5 4 5 6 11 9\n";
        out2 << "11 5 6 7 8 9 10\n";
        out2.close();
    }

    void TearDown() override {
        std::filesystem::remove(conffile1);
        std::filesystem::remove(conffile2);
    }
};

class RuleFiles : public ::testing::Test {
  protected:
    std::string rulefile1 = "rule1.rule";
    std::string rulefile2 = "rule2.rule";
    std::string rulefile3 = "rule3.rule";
    std::string rulefile4 = "rule4.rule";

    void SetUp() override {
        std::ofstream out1(rulefile1);
        out1 << "\n";
        out1 << "2 1 2 2\n";
        out1 << "1 5 5 2 -1\n";
        out1 << "2 5 0 1 -1\n";
        out1.close();
        std::ofstream out2(rulefile2);
        out2 << "\n";
        out2 << "6 1 2 1\n";
        out2 << "1 7 7 5 4 3 2 6 -1\n";
        out2 << "2 7 0 1 3 -1 6\n";
        out2 << "3 5 5 2 1 4 -1\n";
        out2 << "4 5 6 3 1 5 -1\n";
        out2 << "5 5 5 4 1 -1\n";
        out2 << "6 5 5 1 2 -1\n";
        out2.close();
        std::ofstream out3(rulefile3);
        out3 << "\n";
        out3 << "6 1 2 1\n";
        out3 << "1 7 7 4 6 2 3 -1\n";
        out3 << "2 7 0 3 1 6 -1\n";
        out3 << "3 5 5 1 2 -1\n";
        out3 << "4 6 6 5 6 1 -1\n";
        out3 << "5 5 5 6 4 -1\n";
        out3 << "6 5 5 2 1 4 5 -1\n";
        out3.close();
        std::ofstream out4(rulefile4);
        out4 << "\n";
        out4 << "8 1 2 1\n";
        out4 << "1 7 7 3 4 2 6 -1\n";
        out4 << "2 7 7 7 6 1 4 5 -1\n";
        out4 << "3 5 5 4 1 -1\n";
        out4 << "4 7 7 5 2 1 3 -1\n";
        out4 << "5 6 0 2 4 -1\n";
        out4 << "6 5 5 1 2 7 8 -1\n";
        out4 << "7 6 6 8 6 2 -1\n";
        out4 << "8 7 0 6 7 -1\n";
        out4.close();
    }

    void TearDown() override {
        std::filesystem::remove(rulefile1);
        std::filesystem::remove(rulefile2);
        std::filesystem::remove(rulefile3);
        std::filesystem::remove(rulefile4);
    }
};
