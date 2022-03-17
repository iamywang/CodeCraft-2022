#include <iostream>
#include "tools.hpp"
#include "global.hpp"
#include "DataIn.hpp"


int main()
{
    g_qos_constraint = read_qos_constraint();
    read_demand(g_demand);

    read_qos(g_qos);

    read_site_bandwidth(g_site_bandwidth);

    return 0;
}
