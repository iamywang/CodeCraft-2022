#include "global.hpp"

int g_qos_constraint = 0;
SITE_BANDWIDTH g_site_bandwidth{};
QOS g_qos{};
int64_t g_start_time = 0;
const int G_TOTAL_DURATION = 298;//秒
namespace global
{
    DEMAND g_demand{};
}