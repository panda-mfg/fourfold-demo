#include "degree.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

bool Degree::fixed(void) const { return lower == upper; }

bool Degree::disjoint(const Degree &degree0, const Degree &degree1) {
    return degree0.upper < degree1.lower || degree1.upper < degree0.lower;
}

bool Degree::has_intersection(const Degree &degree0, const Degree &degree1) {
    return !Degree::disjoint(degree0, degree1);
}

Degree Degree::intersection(const Degree &degree0, const Degree &degree1) {
    return Degree(std::max(degree0.lower, degree1.lower), std::min(degree0.upper, degree1.upper));
}

bool Degree::include(const Degree &degree0, const Degree &degree1) {
    return degree0.lower <= degree1.lower && degree1.upper <= degree0.upper;
}
