#include "cartwheel.hpp"
#include "combine_cartwheel.hpp"
#include "configuration.hpp"
#include "pseudo_configuration.hpp"
#include "pseudo_triangulation.hpp"
#include "rule.hpp"
#include <boost/program_options.hpp>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

using namespace boost::program_options;
namespace fs = std::filesystem;

bool existArgs(const variables_map &vm, const vector<string> &args) {
    for (const auto &arg : args) {
        if (!vm.count(arg)) {
            spdlog::warn(fmt::format("Specify {}.", arg));
            return false;
        }
    }
    return true;
}

int main(const int ac, const char *const *const av) {
    using namespace boost::program_options;
    options_description description("Options");
    // clang-format off
    description.add_options()
        ("combine_rules", "Combine rules")
        ("enum_wheels", "Enumerate wheels")
        ("enum_cartwheels", "Enumerate cartwheels")
        ("check_deg8", "Combine cartwheels with degree 8 center")
        ("check_7triangle", "Combine cartwheels to form a 7-triangle")
        ("check_deg7", "Combine cartwheels with degree 7 center")
        ("confdir,C", value<string>(), "A directory containing configuration files")
        ("ruledir,R", value<string>(), "A directory containing rule files")
        ("combined_ruledir,S", value<string>(), "A directory containing combined rule files")
        ("cartwheeldir,W", value<string>(), "A directory containing cartwheel files")
        ("wheel,w", value<string>(), "A wheel file")
        ("degree,d", value<int>(), "Degree of the center vertex of wheels")
        ("outdir,o", value<string>(), "Output directory")
        ("help,H", "Display options")
        ("verbosity,v", value<int>()->default_value(0), "1 for debug, 2 for trace");
    // clang-format on

    variables_map vm;
    store(parse_command_line(ac, av, description), vm);
    notify(vm);

    if (vm.count("help")) {
        description.print(std::cout);
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

    if (vm.count("combine_rules")) {
        if (!existArgs(vm, {"ruledir", "confdir", "outdir"})) {
            exit(1);
        }
        string ruledir = vm["ruledir"].as<string>();
        string confdir = vm["confdir"].as<string>();
        string outdir  = vm["outdir"].as<string>();
        run_combine_rules(confdir, ruledir, outdir);
    }
    if (vm.count("enum_wheels")) {
        if (!existArgs(vm, {"degree", "confdir", "ruledir", "combined_ruledir", "outdir"})) {
            exit(1);
        }
        int degree              = vm["degree"].as<int>();
        string confdir          = vm["confdir"].as<string>();
        string ruledir          = vm["ruledir"].as<string>();
        string combined_ruledir = vm["combined_ruledir"].as<string>();
        string outdir           = vm["outdir"].as<string>();
        run_enum_wheels(degree, confdir, ruledir, combined_ruledir, outdir);
    }
    if (vm.count("enum_cartwheels")) {
        if (!existArgs(vm, {"wheel", "confdir", "ruledir", "combined_ruledir", "outdir"})) {
            exit(1);
        }
        string wheel_file       = vm["wheel"].as<string>();
        string confdir          = vm["confdir"].as<string>();
        string ruledir          = vm["ruledir"].as<string>();
        string combined_ruledir = vm["combined_ruledir"].as<string>();
        string outdir           = vm["outdir"].as<string>();
        run_enum_cartwheels(wheel_file, confdir, ruledir, combined_ruledir, outdir);
    }
    if (vm.count("check_deg8")) {
        if (!existArgs(vm, {"cartwheeldir", "confdir"})) {
            exit(1);
        }
        string cartwheeldir = vm["cartwheeldir"].as<string>();
        string confdir      = vm["confdir"].as<string>();
        run_check_deg8(cartwheeldir, confdir);
    }
    if (vm.count("check_7triangle")) {
        if (!existArgs(vm, {"cartwheeldir", "confdir"})) {
            exit(1);
        }
        string cartwheeldir = vm["cartwheeldir"].as<string>();
        string confdir      = vm["confdir"].as<string>();
        run_check_7triangle(cartwheeldir, confdir);
    }
    if (vm.count("check_deg7")) {
        if (!existArgs(vm, {"cartwheeldir", "confdir"})) {
            exit(1);
        }
        string cartwheeldir = vm["cartwheeldir"].as<string>();
        string confdir      = vm["confdir"].as<string>();
        run_check_deg7(cartwheeldir, confdir);
    }
    return 0;
}