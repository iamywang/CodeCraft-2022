#include "global.hpp"
int g_qos_constraint = 0;
int g_minimum_cost = 0;
SITE_BANDWIDTH g_site_bandwidth{};
QOS g_qos{};
int64_t g_start_time = 0;
const int G_TOTAL_DURATION = 285;//秒
DEMAND DEMAND::s_demand{};
MyUtils::Thread::ThreadPool g_thread_pool(NUM_THREAD);

namespace global
{
    // DEMAND g_demand{};
    DEMAND& g_demand = DEMAND::s_demand;
}