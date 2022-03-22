#include <iostream>
#include "utils/tools.hpp"
#include "global.hpp"
#include "data_in_out/DataIn.hpp"
#include "utils/ProcessTimer.hpp"
#include "optimize/optimize_interface.hpp"

extern void write_result(const std::vector<ANSWER> &X_results);
extern bool verify(const std::vector<ANSWER> &X_results);

int main()
{
    MyUtils::MyTimer::ProcessTimer timer;

    g_qos_constraint = read_qos_constraint();
    read_demand(global::g_demand);
    read_qos(g_qos);
    read_site_bandwidth(g_site_bandwidth);

    std::vector<ANSWER> X_results;

    {
        const int num = 200;
        for (int i = 0; i < global::g_demand.demand.size(); i += num)
        {
            optimize::g_demand.clear();

            for (int j = 0; j < num && j + i < global::g_demand.demand.size(); j++)
            {
                optimize::g_demand.demand.push_back(global::g_demand.demand[i + j]);
                optimize::g_demand.mtime.push_back(global::g_demand.mtime[i + j]);
            }

            if (optimize::g_demand.demand.size() == 0)
            {
                continue;
            }

            std::vector<ANSWER> X_results_tmp;
            if (optimize::solve(500, true, X_results_tmp) == 0)
            {
                for (int j = 0; j < X_results_tmp.size(); j++)
                {
                    X_results.push_back(X_results_tmp[j]);
                }
            }
            else
            {
                printf("%d solve failed\n", i);
                exit(-1);
            }
        }
    }

    optimize::g_demand.clear();
    optimize::g_demand.demand = global::g_demand.demand;
    optimize::g_demand.mtime = global::g_demand.mtime;

    verify(X_results);

    if (optimize::solve(1000, false, X_results) == 0)
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
