#include "./tools.hpp"
#include <iostream>
#include <numeric>
#include <cmath>
#include "./utils.hpp"
#include "../optimize/optimize_interface.hpp"

using namespace std;

int calculate_price(const vector<ANSWER> &X_results)
{
    vector<vector<int>> real_site_flow(g_qos.site_name.size(), vector<int>(optimize::g_demand.mtime.size(), 0));
    for (int t = 0; t < X_results.size(); t++)
    {
        const auto &X = X_results[t];
        for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
        {
            real_site_flow[site_id][t] = X.sum_flow_site[site_id];
        }
    }

    for (auto &v : real_site_flow)
    {
        //从小到大排序
        std::sort(v.begin(), v.end());
    }

    int quantile_idx = calculate_quantile_index(0.95, optimize::g_demand.mtime.size());
    int sum = 0;
    for (int i = 0; i < real_site_flow.size(); i++)
    {
        sum += real_site_flow[i][quantile_idx];
    }
    return sum;
}

bool verify(const vector<ANSWER> &X_results)
{
    for (const auto &X : X_results)
    {
        const auto &flow = X.flow;

        for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
        {
            int sum = MyUtils::Tools::sum_column(X.flow, site_id);
            if (sum != X.sum_flow_site[site_id])
            {
                printf("%s: ", X.mtime.c_str());
                printf("X.sum_flow_site[%d] = %d, but real sum = %d\n", site_id, X.sum_flow_site[site_id], sum);
                return false;
            }
            else if (X.sum_flow_site[site_id] > g_site_bandwidth.bandwidth[site_id])
            {
                printf("%s: ", X.mtime.c_str());
                printf("X.sum_flow_site[%d] = %d, but real bandwidth[%d] = %d\n", site_id, X.sum_flow_site[site_id], site_id, g_site_bandwidth.bandwidth[site_id]);
                return false;
            }
        }

        for (int client_id = 0; client_id < flow.size(); client_id++)
        {
            for (int j = 0; j < flow[client_id].size(); j++)
            {
                if ((flow[client_id][j] && g_qos.qos[j][client_id] <= 0) || flow[client_id][j] < 0)
                {
                    cout << X.mtime << " client " << g_qos.client_name[client_id] << " server " << g_qos.site_name[j] << endl;
                    return false;
                }
            }

            int sum = std::accumulate(flow[client_id].begin(), flow[client_id].end(), 0); //客户的流量总和
            if (sum != optimize::g_demand.demand[optimize::g_demand.get(X.mtime)][client_id])
            {
                cout << X.mtime << " " << client_id << " " << sum << " " << optimize::g_demand.demand[optimize::g_demand.get(X.mtime)][client_id] << endl;
                cout << "sum is not equal demand" << endl;
                return false;
            }
        }
    }

    {
        //下面计算成本
        int price = calculate_price(X_results);
        printf("verify:总成本是 %d\n", price);
    }
    return true;
}