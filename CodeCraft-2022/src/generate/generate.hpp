#pragma once

#include <iostream>
#include <functional>
#include <numeric>
#include <algorithm>
#include "../utils/tools.hpp"
#include "../utils/Verifier.hpp"
#include "../utils/Graph/MaxFlow.hpp"
#include "../utils/utils.hpp"

using namespace std;

namespace generate
{
    void allocate_flow_to_stream(vector<ANSWER> &X_results)
    {
        // 计算初始解中95%分位的流量（考虑g_minimum_cost，作为流量分配的阈值）
        vector<int> quantile_flow(g_num_server, 0);
        int quantile_idx = calculate_quantile_index(0.95, X_results.size());

        vector<vector<int>> real_site_flow(g_qos.site_name.size(), vector<int>(X_results.size(), 0));
        {
            auto task = [&](int t)
            {
                const auto &X = X_results[t];
                for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
                {
                    real_site_flow[site_id][t] = X.sum_flow_site[site_id];
                }
            };
            auto rets = parallel_for(0, X_results.size(), task);
            // for (int t = 0; t < X_results.size(); t++)
            // {
            //     const auto &X = X_results[t];
            //     for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
            //     {
            //         real_site_flow[site_id][t] = X.sum_flow_site[site_id];
            //     }
            // }
            for(auto& i : rets)
            {
                i.get();
            }
        }
        {
            auto task = [&real_site_flow](int t)
            {
                std::sort(real_site_flow[t].begin(), real_site_flow[t].end());
            };
            auto rets = parallel_for(0, g_qos.site_name.size(), task);
            for(auto&i : rets)
            {
                i.get();
            }
            // for (auto &v : real_site_flow)
            // {
            //     //从小到大排序
            //     std::sort(v.begin(), v.end());
            // }
        }

        for (int i = 0; i < real_site_flow.size(); i++)
        {
            const int flow = real_site_flow[i][quantile_idx];
            if (flow > 0)
                quantile_flow[i] = max(flow, g_minimum_cost);
            else
                quantile_flow[i] = 0;
        }

        // 计算初始解的成本
        // double init_cost = 0;
        // for (int i = 0; i < real_site_flow.size(); i++)
        // {
        //     init_cost += X_results[0].calculate_cost(i, quantile_flow[i]);
        // }
        // printf("generate: initial results total price is %.0f\n", round(init_cost));

        // 根据初始解进行流量分配
        auto task = [&](const int time)
        {
            auto &answer = X_results[time];
            vector<vector<int>> init_flow = answer.flow;

            vector<int> left_bandwidth(g_num_server);
            for (int i = 0; i < g_num_server; i++)
            {
                left_bandwidth[i] = g_site_bandwidth.bandwidth[i] - answer.sum_flow_site[i];
            }

            const int stream_nums = global::g_demand.stream_client_demand[time].id_local_stream_2_stream_name.size();

            for (int client_id = 0; client_id < g_num_client; client_id++)
            {
                // 当前客户端的流
                for (int stream_id = 0; stream_id < stream_nums; stream_id++)
                {
                    const int stream_client_demand = global::g_demand.stream_client_demand[time].stream_2_client_demand[stream_id][client_id];
                    if (stream_client_demand == 0)
                    {
                        continue;
                    }

                    // 按照flow分配给边缘节点，满足增加流量后不超过阈值
                    int most_match_site_id = -1;
                    double match_percent = 0;
                    int added_flow = 0;
                    for (int site_id = 0; site_id < g_num_server; site_id++)
                    {
                        if (g_qos.qos[site_id][client_id] == 0)
                        {
                            continue;
                        }

                        // 分配flow，要考虑流量阈值
                        int flow = answer.flow[client_id][site_id];
                        if (g_site_bandwidth.bandwidth[site_id] - left_bandwidth[site_id] + flow < quantile_flow[site_id])
                        {
                            int new_flow = quantile_flow[site_id] - g_site_bandwidth.bandwidth[site_id] + left_bandwidth[site_id];
                            flow = max(flow, int(new_flow * 1));
                        }

                        if (stream_client_demand <= flow)
                        {
                            if (double(stream_client_demand) / double(flow) > match_percent)
                            {
                                match_percent = double(stream_client_demand) / double(flow);
                                most_match_site_id = site_id;
                                added_flow = flow - answer.flow[client_id][site_id];
                            }
                        }
                    }

                    if (most_match_site_id != -1)
                    {
                        answer.set(stream_id, client_id, most_match_site_id);
                        answer.flow[client_id][most_match_site_id] = answer.flow[client_id][most_match_site_id] - stream_client_demand + added_flow;
                        left_bandwidth[most_match_site_id] -= stream_client_demand;
                    }

                    // 会超过原有阈值的流量
                    // 1. 分配给成本为0的节点
                    // 2. 让某个节点成本超过阈值
                    else
                    {
                        int most_match_zero_site_id = -1;
                        int most_match_over_site_id = -1;
                        double match_zero_percent = 0;
                        double match_over_percent = 0;

                        for (int site_id = 0; site_id < g_num_server; site_id++)
                        {
                            if (g_qos.qos[site_id][client_id] == 0)
                            {
                                continue;
                            }

                            const int left = left_bandwidth[site_id];
                            if (stream_client_demand <= left)
                            {
                                if (quantile_flow[site_id] == 0)
                                {
                                    if (double(stream_client_demand) / double(g_site_bandwidth.bandwidth[site_id]) < match_zero_percent || match_percent == 0)
                                    {
                                        match_zero_percent = double(stream_client_demand) / double(g_site_bandwidth.bandwidth[site_id]);
                                        most_match_zero_site_id = site_id;
                                    }
                                }
                                else
                                {
                                    if (double(stream_client_demand) / double(g_site_bandwidth.bandwidth[site_id]) < match_over_percent || match_percent == 0)
                                    {
                                        match_over_percent = double(stream_client_demand) / double(g_site_bandwidth.bandwidth[site_id]);
                                        most_match_over_site_id = site_id;
                                    }
                                }
                            }
                        }

                        /*

                        // 如果分配给成本为0的节点，带来的cost
                        double cost_zero = 0;
                        if (stream_client_demand < g_minimum_cost)
                            cost_zero = g_minimum_cost;
                        else
                        {
                            int flow_sub_cost = stream_client_demand - g_minimum_cost;
                            cost_zero = pow(double(flow_sub_cost), 2) / double(g_site_bandwidth.bandwidth[most_match_zero_site_id]) + stream_client_demand;
                        }

                        // 如果分配给成本超过阈值的节点，带来的cost
                        double cost_over = 0;
                        int flow_pre = g_site_bandwidth.bandwidth[most_match_over_site_id] - left_bandwidth[most_match_over_site_id];
                        int flow_now = flow_pre + stream_client_demand;
                        int flow_sub_cost_pre = flow_pre - g_minimum_cost;
                        int flow_sub_cost = flow_now - g_minimum_cost;
                        if (flow_sub_cost_pre > 0)
                            cost_over = (pow(double(flow_sub_cost), 2) / double(g_site_bandwidth.bandwidth[most_match_over_site_id]) + flow_now) - (pow(double(flow_sub_cost_pre), 2) / double(g_site_bandwidth.bandwidth[most_match_over_site_id]) + flow_pre);
                        else
                            cost_over = (pow(double(flow_sub_cost), 2) / double(g_site_bandwidth.bandwidth[most_match_over_site_id]) + flow_now) - g_minimum_cost;

                        if (most_match_zero_site_id != -1 && cost_zero - cost_over < 0)

                        */

                        if (most_match_zero_site_id != -1)
                        {
                            answer.set(stream_id, client_id, most_match_zero_site_id);
                            left_bandwidth[most_match_zero_site_id] -= stream_client_demand;
                        }
                        else if (most_match_over_site_id != -1)
                        {
                            answer.set(stream_id, client_id, most_match_over_site_id);
                            answer.flow[client_id][most_match_over_site_id] -= min(stream_client_demand, answer.flow[client_id][most_match_over_site_id]);
                            left_bandwidth[most_match_over_site_id] -= stream_client_demand;
                        }
                        else
                        {
                            printf("no match site: %d %d %d %d\n", time, client_id, stream_id, stream_client_demand);
                        }
                    }
                }

                // 更新剩余带宽
                for (int site_id = 0; site_id < g_num_server; site_id++)
                {
                    left_bandwidth[site_id] += init_flow[client_id][site_id];
                }
            }

            // 更新flow, sum_flow_site和cost
            // answer.flow.resize(g_num_client, vector<int>(g_num_server, 0));
            answer.flow = vector<vector<int>>(g_num_client, vector<int>(g_num_server, 0));
            for (int client_id = 0; client_id < g_num_client; client_id++)
            {
                for (int stream_id = 0; stream_id < stream_nums; stream_id++)
                {
                    const int stream_client_demand = global::g_demand.stream_client_demand[time].stream_2_client_demand[stream_id][client_id];
                    const int site_id = answer.stream2server_id[stream_id][client_id];
                    answer.flow[client_id][site_id] += stream_client_demand;
                }
            }

            for (int site_id = 0; site_id < g_num_server; site_id++)
            {
                int tmp = MyUtils::Tools::sum_column(answer.flow, site_id);
                answer.sum_flow_site[site_id] = tmp;
                answer.cost[site_id] = answer.calculate_cost(site_id, tmp);
            }
        };
        // for (int time = 0; time < X_results.size(); time++)
        // {
        //     task(time);
        // }

        auto vec = parallel_for(0, X_results.size(), task);
        for (auto &ret : vec)
        {
            ret.get();
        }
    }
} // namespace generate
