#pragma once

#include <iostream>
#include <functional>
#include <numeric>
#include <algorithm>
#include "../utils/tools.hpp"
#include "../utils/Verifier.hpp"
#include "../utils/Graph/MaxFlow.hpp"

using namespace std;

namespace generate
{
    void allocate_flow_to_stream(vector<ANSWER> &X_results)
    {
        for (int time = 0; time < X_results.size(); time++)
        {
            auto &answer = X_results[time];
            vector<vector<int>> init_flow = answer.flow;

            vector<int> left_bandwidth(g_num_server);
            for (int i = 0; i < g_num_server; i++)
            {
                left_bandwidth[i] = g_site_bandwidth.bandwidth[i] - answer.sum_flow_site[i];
            }

            const int stream_nums = global::g_demand.stream_client_demand[time].id_local_stream_2_stream_name.size();
            answer.init(stream_nums);

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

                    // 按照flow分配给边缘节点，找到一个最匹配的
                    int most_match_site_id = -1;
                    double match_percent = 0;
                    for (int site_id = 0; site_id < g_num_server; site_id++)
                    {
                        if (g_qos.qos[site_id][client_id] == 0)
                        {
                            continue;
                        }

                        // 分配flow，要考虑g_minimum_cost
                        int flow = answer.flow[client_id][site_id];
                        // if (flow > 0)
                        // {
                        //     if (flow < g_minimum_cost)
                        //         flow = g_minimum_cost;
                        // }
                        // else
                        // {
                        //     if (answer.sum_flow_site[site_id] > 0)
                        //         flow = g_minimum_cost;
                        // }

                        // qos满足，可以分配
                        if (stream_client_demand <= flow)
                        {
                            if (double(stream_client_demand) / double(flow) > match_percent)
                            {
                                match_percent = double(stream_client_demand) / double(flow);
                                most_match_site_id = site_id;
                            }
                        }
                    }

                    if (most_match_site_id != -1)
                    {
                        answer.set(stream_id, client_id, most_match_site_id);
                        answer.flow[client_id][most_match_site_id] -= stream_client_demand;
                        left_bandwidth[most_match_site_id] -= stream_client_demand;
                    }

                    // 按照left_bandwidth分配给边缘节点，找到一个最匹配的
                    if (most_match_site_id == -1)
                    {
                        most_match_site_id = -1;
                        match_percent = 0;
                        for (int site_id = 0; site_id < g_num_server; site_id++)
                        {
                            if (g_qos.qos[site_id][client_id] == 0)
                            {
                                continue;
                            }

                            // qos满足，可以分配
                            const int left = left_bandwidth[site_id];
                            if (stream_client_demand <= left)
                            {
                                if (double(stream_client_demand) / double(left) > match_percent)
                                {
                                    match_percent = double(stream_client_demand) / double(left);
                                    most_match_site_id = site_id;
                                }
                            }
                        }

                        if (most_match_site_id != -1)
                        {
                            answer.set(stream_id, client_id, most_match_site_id);
                            left_bandwidth[most_match_site_id] -= stream_client_demand;
                        }
                    }

                    if (most_match_site_id == -1)
                    {
                        cout << "no match site" << endl;
                    }
                }

                // 更新剩余带宽
                for (int site_id = 0; site_id < g_num_server; site_id++)
                {
                    left_bandwidth[site_id] += init_flow[client_id][site_id];
                }
            }

            // 更新一遍flow
            for (int site_id = 0; site_id < g_num_server; site_id++)
            {
                for (int client_id = 0; client_id < g_num_client; client_id++)
                {
                    answer.flow[client_id][site_id] = 0;
                }
            }

            for (int stream_id = 0; stream_id < stream_nums; stream_id++)
            {
                for (int client_id = 0; client_id < g_num_client; client_id++)
                {
                    const int stream_client_demand = global::g_demand.stream_client_demand[time].stream_2_client_demand[stream_id][client_id];
                    answer.flow[client_id][answer.stream2server_id[stream_id][client_id]] += stream_client_demand;
                }
            }

            // 更新sum_flow_site
            for (int site_id = 0; site_id < g_num_server; site_id++)
            {
                int tmp = MyUtils::Tools::sum_column(answer.flow, site_id);
                answer.sum_flow_site[site_id] = tmp;
            }
        }
    }
} // namespace generate
