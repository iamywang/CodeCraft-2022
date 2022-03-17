void test_datain()
{
    g_qos_constraint = read_qos_constraint();
    // printf("g_qos_constraint: %d\n", g_qos_constraint);
    read_demand(g_demand);
    // for(int i = 0; i < g_demand.mtime.size(); i++)
    // {
    //     printf("g_demand.mtime[%d]: %s ", i, g_demand.mtime[i].c_str());
    //     for(auto& d : g_demand.demand[i])
    //     {
    //         printf("%d ", d);
    //     }
    //     printf("\n");
    // }

    read_qos(g_qos);
    // for(auto &c : g_qos.client_name)
    // {
    //     printf("g_qos.client_name: %s\n", c.c_str());
    // }
    // for(int i = 0; i < g_qos.site_name.size(); i++)
    // {
    //     printf("g_qos.site_name[%d]: %s ", i, g_qos.site_name[i].c_str());
    //     for(auto& b : g_qos.qos[i])
    //     {
    //         printf("%d ", b);
    //     }
    //     printf("\n");
    // }

    read_site_bandwidth(g_site_bandwidth);
    // for(int i = 0; i < g_site_bandwidth.site_name.size(); i++)
    // {
    //     printf("g_site_bandwidth.site_name[%d]: %s %d", i, g_site_bandwidth.site_name[i].c_str(), g_site_bandwidth.bandwidth[i]);
    //     printf("\n");
    // }
}