// Audit adapter: generate only the planar tables needed for two sample checks.
// The upstream CLI also prepares other surfaces, which are unnecessary here.
#include "generate_kempes.hpp"
#include "generate_colors.hpp"

int main() {
    std::filesystem::create_directories("kempes/plan");
    for (int s = 1; s <= 7; ++s) {
        auto cases = GetPlanarKempes(s);
        std::ofstream out("kempes/plan/kempes_" + std::to_string(s) + ".txt");
        out << cases.size() << '\n';
        for (const auto& c : cases) out << c << '\n';
        if (!out) return 1;
    }
    GenerateColors(14);
}
