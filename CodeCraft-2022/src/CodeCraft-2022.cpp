#include <iostream>
#include "utils/tools.hpp"
#include "global.hpp"
#include "data_in_out/DataIn.hpp"
#include "utils/ProcessTimer.hpp"
extern int solve(std::vector<ANSWER> &X_results);
extern void write_result(const std::vector<ANSWER> &X_results);

int main()
{
    MyUtils::MyTimer::ProcessTimer timer;
    g_qos_constraint = read_qos_constraint();
    // printf("g_qos_constraint: %d\n", g_qos_constraint);
    read_demand(g_demand);

    read_qos(g_qos);
    // printf("边缘节点数量：%d\n", g_qos.site_name.size());
    // printf("客户节点数量：%d\n", g_qos.client_name.size());

    read_site_bandwidth(g_site_bandwidth);
    // for(auto &c : g_qos.client_name)
    // {
    //     printf("g_qos.client_name: %s\n", c.c_str());
    // }
    // for(int i = 0; i < g_qos.site_name.size(); i++)
    // {
    //     // printf("g_qos.site_name[%d]: %s ", i, g_qos.site_name[i].c_str());
    //     for(auto& b : g_qos.qos[i])
    //     {
    //         printf("%04d ", b);
    //     }
    //     printf("\n");
    // }
    std::vector<ANSWER> X_results;
    if(solve(X_results) == 0)
    {
        write_result(X_results);
    }
    else
    {
        printf("solve failed\n");
    }
    // write_result(X_results);
    printf("总耗时：%d ms\n", timer.duration());
    return 0;
}
