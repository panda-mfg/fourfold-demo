#include "cartwheel.hpp"
#include "configuration.hpp"

vector<CartWheel> delete_degree_from_k_to_9(const vector<CartWheel> &cartwheels, int k);
vector<CartWheel> delete_7triangle(const vector<CartWheel> &cartwheels);
Configuration get_7triangle(void);

// A vertex of degree 8
void run_check_deg8(const string &cartwheeldir, const string &confdir);
void check_deg8(const vector<CartWheel> &cartwheels, const vector<Configuration> &confs);
void check88(const CartWheel &cartwheel, const vector<int> &darts8,
             const vector<CartWheel> &cartwheels, const vector<Configuration> &confs);
void check87(const CartWheel &cartwheel, const vector<int> &darts7,
             const vector<CartWheel> &cartwheels, const vector<Configuration> &confs);
void check787(const CartWheel &cartwheel, const vector<int> &darts7,
              const vector<CartWheel> &cartwheels, const vector<Configuration> &confs);
// X
PseudoConfiguration getX(void);
bool containX(const PseudoConfiguration &pc, int v);

// Maximal degree 7
// 7-triangle
void run_check_7triangle(const string &cartwheeldir, const string &confdir);
void check_7triangle(const vector<CartWheel> &cartwheels, const vector<Configuration> &confs);
// A vertex of degree 7
void run_check_deg7(const string &cartwheeldir, const string &confdir);
void check_deg7(const vector<CartWheel> &cartwheels, vector<Configuration> &confs);
void check77(const CartWheel &cartwheel, const vector<int> &darts7,
             const vector<CartWheel> &cartwheels, const vector<Configuration> &confs);
void check777(const CartWheel &cartwheel, const vector<int> &darts7,
              const vector<CartWheel> &cartwheels, const vector<Configuration> &confs);
