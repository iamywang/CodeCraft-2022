#pragma once
#include "./tools.hpp"
#include <iostream>
#include <numeric>
#include <cmath>
#include "./utils.hpp"
#include "../optimize/optimize_interface.hpp"

using namespace std;
class Verifier
{
private:
    DEMAND &m_demand;

public:
    Verifier(DEMAND &demand) : m_demand(demand) {}
    ~Verifier() {}

private:
    double calculate_price(const vector<ANSWER> &X_results)
    {
        vector<vector<int>> real_site_flow(g_qos.site_name.size(), vector<int>(this->m_demand.mtime.size(), 0));
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

        int quantile_idx = calculate_quantile_index(0.95, this->m_demand.mtime.size());
        double sum = 0;
        for (int i = 0; i < real_site_flow.size(); i++)
        {
            if (sum > 0 && sum <= g_minimum_cost)
            {
                sum += g_minimum_cost;
            }
            else if (sum > g_minimum_cost)
            {
                const int flow = real_site_flow[i][quantile_idx];
                const int bandwidth = g_site_bandwidth.bandwidth[i];
                sum += pow(double(flow - g_minimum_cost), 2) / bandwidth + flow;
            }
        }
        return sum;
    }

public:
    bool verify(const vector<ANSWER> &X_results)
    {
        for (int time = 0; time < X_results.size(); time++)
        {
            const auto &X = X_results[time];
            const auto &stream_client = X.stream2server_id;
            std::vector<int> server_sum_flow;
            server_sum_flow.resize(g_qos.site_name.size(), 0);

            for (int stream_id = 0; stream_id < stream_client.size(); stream_id++)
            {
                for (int client_id = 0; client_id < stream_client[stream_id].size(); client_id++)
                {
                    const int server_id = stream_client[stream_id][client_id];
                    const int stream_client_demand = global::g_demand.demand[time].stream_2_client_demand[stream_id][client_id];
                    bool isAlloc = false;
                    if (stream_client_demand > 0)
                    {
                        isAlloc = true;
                    }

                    if (server_id >= 0)
                    {
                        if (isAlloc == false)
                        {
                            printf("%s: ", X.mtime.c_str());
                            printf("X.stream2server_id[%d][%d] = %d, but real demand = 0\n", stream_id, client_id, server_id);
                            return false;
                        }
                        server_sum_flow[server_id] += stream_client_demand;
                    }
                    else if (isAlloc == true && server_id == -1)
                    {
                        printf("%s: ", X.mtime.c_str());
                        printf("X.stream2server_id[%d][%d] is not satisfied (demand is %d)\n", stream_id, client_id, stream_client_demand);
                        return false;
                    }
                }
            }

            for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
            {
                int sum = server_sum_flow[site_id];
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
        }

        {
            //下面计算成本
            int price = calculate_price(X_results);
            printf("verify:total price is %d\n", price);
        }
        return true;
    }
};
