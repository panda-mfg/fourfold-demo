#include "generate_kempes.hpp"
#include "generate_colors.hpp"
#include "check_reducibility.hpp"
#include "duality.hpp"

#include <boost/tokenizer.hpp>
#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>
#include <fmt/core.h>
#include <exception>

int main(const int ac, const char* const* const av) {
    using namespace boost::program_options;
    options_description description("Options");
    description.add_options()
        ("kempe,k", value<int>()->default_value(0), "Number of kempe files to generate")
        ("color,c", value<int>()->default_value(0), "Number of color files to generate")
        ("input,i", value<string>(), "The file to evaluate")
        ("duality,d", "Convert a conf file to dconf")
        ("planar,l", "Evaluate the dconf file in planar mode")
        ("apex,a", "Evaluate the dconf file in apex mode")
        ("toroidal,t", "Evaluate the dconf file in toroidal mode")
        ("annular,n", "Evaluate a nconf file")
        ("edge-set,s", value<string>()->default_value(""), "Test only one edge contraction set (Does not check for edge validity)")
        ("help,H", "Display options")
        ("verbosity,v", value<int>()->default_value(0), "1 for debug, 2 for trace")
        ("chalt,h", value<int>()->default_value(0), "How to halt after a successful contraction has been found. (0: halt immediately, 1: halt after searching all conts with same size, 2: do not halt)")
        ("cmin", value<int>()->default_value(1), "Min number of edges to contract")
        ("cmax,m", value<int>()->default_value(0), "Max number of edges to contract in C-red. check (0 for no limit)")
        ("feasibles,f", value<string>()->default_value(""), "The feasible file (a file where the feasibility information of the configuration is stored)")
        ("write-f,w", "Store info into a feasible file")
        ("read-f,r", "Read from feasible file (Skip D-reducibility check)")
        ("without-d", "Output the feasible file without the D-reducibility check (For debugging purposes)")
        ("rotate-f", "Rotate the coloring info for feasible info (For debugging purposes, not to be used for read-f option)");

    variables_map vm;
    store(parse_command_line(ac, av, description), vm);
    notify(vm);

    if (vm.count("help")) {
        description.print(cout);
        return 0;
    }
    if (vm.count("verbosity")) {
        auto v = vm["verbosity"].as<int>();
        if (v == 1) {
            spdlog::set_level(spdlog::level::debug);
        }
        if (v == 2) {
            spdlog::set_level(spdlog::level::trace);
        }
    }
    if (vm.count("kempe")) {
        auto k = vm["kempe"].as<int>();
        if (k > 0) {
            spdlog::info("Generating {} kempe files...", k);
            GenerateKempes(k);
        }
    }
    if (vm.count("color")) {
        auto c = vm["color"].as<int>();
        if (c > 0) {
            spdlog::info("Generating {} color files...", c);
            GenerateColors(c);
        }
    }
    if (vm.count("input")) {
        auto fileName = vm["input"].as<string>();
        auto duality = vm.count("duality") > 0;
        auto planar = vm.count("planar") > 0;
        auto apex = vm.count("apex") > 0;
        auto toroidal = vm.count("toroidal") > 0;
        auto annular = vm.count("annular") > 0;
        auto edgeSetString = vm["edge-set"].as<string>();
        auto haltNum = vm["chalt"].as<int>();
        auto haltType = haltNum == 0 ? HaltImmediately : haltNum == 1 ? HaltAfterSameSize : NoHalt;
        auto contMin = vm["cmin"].as<int>();
        auto contMax = vm["cmax"].as<int>();
        auto feasibleFile = vm["feasibles"].as<string>();
        auto writeToFeasible = vm.count("write-f") > 0;
        auto readFromFeasible = vm.count("read-f") > 0;
        auto withoutD = vm.count("without-d") > 0;
        auto rotateFeasible = vm.count("rotate-f") > 0;

        bool hasEdgeSet = edgeSetString.size() > 0;
        vector<int> edgeSet;
        if (hasEdgeSet) {
            boost::char_separator<char> sep("+,");
            boost::tokenizer<boost::char_separator<char>> tokens(edgeSetString, sep);
            for (std::string s : tokens) {
                edgeSet.push_back(std::stoi(s));
            }
        }
        if (duality) {
            OutputDuality(fileName, cout);
        }
        else {
            try {
                if (annular) {
                    EvaluateConf<AnnularCubicConf>(fileName, planar ? Planar : apex ? Apex : toroidal ? Toroidal : Projective, haltType, contMin, contMax, feasibleFile, readFromFeasible, writeToFeasible, rotateFeasible, withoutD, hasEdgeSet, edgeSet, annular);
                }
                else {
                    EvaluateConf<CubicConf>(fileName, planar ? Planar : apex ? Apex : toroidal ? Toroidal : Projective, haltType, contMin, contMax, feasibleFile, readFromFeasible, writeToFeasible, rotateFeasible, withoutD, hasEdgeSet, edgeSet, annular);
                }
            }
            catch (const std::exception& e) {
                spdlog::critical("The program threw an error: {}", e.what());
                spdlog::critical("Terminating.");
            }
        }
    }
    return 0;
}