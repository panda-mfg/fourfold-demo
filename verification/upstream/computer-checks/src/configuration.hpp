#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP
#include "pseudo_configuration.hpp"

struct Configuration : public PseudoConfiguration {
    int dart_id;
    Configuration(int dart_id, int N, const vector<Dart> &darts, const vector<Degree> &degrees)
        : PseudoConfiguration(N, darts, degrees), dart_id(dart_id) {}
    bool operator==(const Configuration &other) const = default;
    static vector<Configuration> from_file(const string &filename);
    static vector<Configuration> get_confs(const string &confdir);
    Configuration mirror(void) const;
};

vector<Configuration> get_mirrors(const vector<Configuration> &confs);
vector<Configuration> extend_from_cut_vertices(int N, int R, const vector<Degree> &degrees,
                                               const vector<vector<int>> &rotations);
vector<pair<int, int>> find_cut_pairs(int N, int R, const vector<vector<int>> &rotations);
PseudoConfiguration remove_ring(int N, int R, const vector<Degree> &degrees,
                                const vector<vector<int>> &rotations, const vector<bool> &remove);
int maximum_degree_dart(const PseudoConfiguration &Z);

#endif // CONFIGURATION_HPP