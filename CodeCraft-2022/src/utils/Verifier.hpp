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

public:
    static int calculate_price(const vector<ANSWER> &X_results)
    {
        int m_current_mtime_count = X_results.size(); //当前要校验的时刻数量
        int m_95_quantile_idx = calculate_quantile_index(0.95, m_current_mtime_count);

        vector<vector<double>> costs(g_num_server, vector<double>(m_current_mtime_count, 0));
        for (int i = 0; i < X_results.size(); i++)
        // for(auto&X : X_results)
        {
            auto &X = X_results[i];
            for (int id_server = 0; id_server < g_num_server; id_server++)
            {
                costs[id_server][i] = ANSWER::calculate_cost(id_server, X.sum_flow_site[id_server]);
                if (int(costs[id_server][i]) != int(X.cost[id_server]))
                {
                    printf("%s: costs[%d][%d] != X.cost[%d]\n", __func__, id_server, X.idx_global_mtime, id_server);
                    exit(-1);
                }
            }
        }

        std::vector<double> costs_vec(m_current_mtime_count, 0);
        {
            // for (auto &v : costs)
            // {
            //     //从小到大排序
            //     std::sort(v.begin(), v.end());
            // }
            auto task = [&costs_vec, &costs, m_95_quantile_idx, m_current_mtime_count](int id_server)
            {
                int quantile_idx = m_95_quantile_idx;
                if(g_90_percent_server_id_set.find(id_server) != g_90_percent_server_id_set.end())
                {
                    quantile_idx = calculate_quantile_index(0.9, m_current_mtime_count);
                }
                nth_element(costs[id_server].begin(),
                            costs[id_server].begin() + quantile_idx,
                            costs[id_server].end());
                costs_vec[id_server] = costs[id_server][quantile_idx];
            };
            auto rets = parallel_for(0, costs.size(), task);
            for (auto &i : rets)
            {
                i.get();
            }
        }

        double sum = 0;
        for (int i = 0; i < g_num_server; i++)
        {
            sum += costs_vec[i];
        }
        return sum;
    }

    static bool verify(const vector<ANSWER> &X_results)
    {
        int current_mtime_count = X_results.size(); //当前要校验的时刻数量
        int quantile_95_idx = calculate_quantile_index(0.95, current_mtime_count);

        for (const auto &X : X_results)
        {
            const auto &flow = X.flow;

            for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
            {
                int sum = MyUtils::Tools::sum_column(X.flow, site_id);
                if (sum != X.sum_flow_site[site_id])
                {
                    printf("%s: ", global::g_demand.mtime[X.idx_global_mtime].c_str());
                    printf("X.sum_flow_site[%d] = %d, but real sum = %d\n", site_id, X.sum_flow_site[site_id], sum);
                    return false;
                }
                else if (X.sum_flow_site[site_id] > g_site_bandwidth.bandwidth[site_id])
                {
                    printf("%s: ", global::g_demand.mtime[X.idx_global_mtime].c_str());
                    printf("X.sum_flow_site[%d] = %d, but real bandwidth[%d] = %d\n", site_id, X.sum_flow_site[site_id], site_id, g_site_bandwidth.bandwidth[site_id]);
                    return false;
                }
                if (int(ANSWER::calculate_cost(site_id, sum)) != int(X.cost[site_id])) //成本不一致，退出
                {
                    printf("%s: ", global::g_demand.mtime[X.idx_global_mtime].c_str());
                    printf("X.cost[%d] = %f, but real cost = %f\n", site_id, X.cost[site_id], ANSWER::calculate_cost(site_id, sum));
                    return false;
                }
            }

            for (int client_id = 0; client_id < flow.size(); client_id++)
            {
                for (int j = 0; j < flow[client_id].size(); j++)
                {
                    if ((flow[client_id][j] && g_qos.qos[j][client_id] <= 0) || flow[client_id][j] < 0)
                    {
                        cout << global::g_demand.mtime[X.idx_global_mtime] << " client " << g_qos.client_name[client_id] << " server " << g_qos.site_name[j] << endl;
                        return false;
                    }
                }

                int sum = std::accumulate(flow[client_id].begin(), flow[client_id].end(), 0); //客户的流量总和
                if (sum != global::g_demand.client_demand[X.idx_global_mtime][client_id])
                {
                    cout << global::g_demand.mtime[X.idx_global_mtime] << " " << client_id << " " << sum << " " << global::g_demand.client_demand[X.idx_global_mtime][client_id] << endl;
                    cout << "sum is not equal demand" << endl;
                    return false;
                }
            }
        }
        return true;
    }

    static bool verify_stream(const vector<ANSWER> &X_results)
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
                    const int stream_client_demand = global::g_demand.stream_client_demand[time].stream_2_client_demand[stream_id][client_id];
                    bool isAlloc = false;
                    if (stream_client_demand > 0)
                    {
                        isAlloc = true;
                    }

                    if (server_id >= 0)
                    {
                        if (isAlloc == false)
                        {
                            // printf("%s: ", X.mtime.c_str());
                            printf("%s: ", global::g_demand.mtime[X.idx_global_mtime].c_str());

                            printf("client[%d]: %s, stream[%d]: %s is allocated to server[%d]: %s, but real demand is: 0\n",
                                   client_id, global::g_demand.client_name[client_id].c_str(),
                                   stream_id, global::g_demand.stream_client_demand[time].id_local_stream_2_stream_name[stream_id].c_str(),
                                   server_id, g_qos.site_name);
                            return false;
                        }
                        server_sum_flow[server_id] += stream_client_demand;
                    }
                    else if (isAlloc == true && server_id == -1)
                    {
                        // printf("%s: ", X.mtime.c_str());
                        printf("%s: ", global::g_demand.mtime[X.idx_global_mtime].c_str());

                        printf("client[%d]: %s, stream[%d]: %s is not allocated to any server, but real demand is: %d\n",
                               client_id, global::g_demand.client_name[client_id].c_str(),
                               stream_id, global::g_demand.stream_client_demand[time].id_local_stream_2_stream_name[stream_id].c_str(),
                               stream_client_demand);
                        return false;
                    }
                }
            }
        }
        return true;
    }
};
