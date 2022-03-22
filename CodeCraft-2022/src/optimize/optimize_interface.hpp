#pragma once

#include "../global.hpp"

namespace optimize
{
    extern DEMAND g_demand;
    int solve(const int num_iteration, const bool is_generate_initial_results, std::vector<ANSWER> &X_results);
}
