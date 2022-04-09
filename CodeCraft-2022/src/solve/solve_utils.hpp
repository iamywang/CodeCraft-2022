#pragma once

#include <vector>
#include "solve_utils.hpp"
#include "../global.hpp"

using namespace std;

namespace solve
{
    class Utils
    {
    private:
        Utils(/* args */) {}
        ~Utils() {}

    public:
        /**
         * @brief
         *
         * @return server_supported_flow 边缘节点在各个时刻可提供的最大流量。每一行都是每个时刻各个边缘节点可以为当前所有客户端提供的最大流量。且按照流量从大到小排序
         */
        static void
        sort_by_demand_and_qos(
            // DEMAND &demand,
                                const std::vector<int> &idx_global_demand,
                               vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_time_vec)
        {
            server_supported_flow_2_time_vec.clear();
            // for (int k = 0; k < demand.client_demand.size(); k++)
            for (int k = 0; k < idx_global_demand.size(); k++)
            {
                vector<SERVER_SUPPORTED_FLOW> server_supported_flow;

                // const auto &line_demand = demand.client_demand[k];
                const auto &line_demand = global::g_demand.client_demand[idx_global_demand[k]];


                for (int server_id = 0; server_id < g_qos.site_name.size(); server_id++) //遍历边缘节点
                {
                    int max_server_support_flow = 0;                                     //边缘节点为当前时刻所有客户可支持的最大流量
                    for (int client_id = 0; client_id < line_demand.size(); client_id++) //遍历每一个客户的需求
                    {
                        if (g_qos.qos[server_id][client_id] > 0) //需要满足QOS
                        {
                            max_server_support_flow += line_demand[client_id];
                        }
                    }

                    max_server_support_flow = std::min(max_server_support_flow,
                                                       g_site_bandwidth.bandwidth[server_id]);

                    server_supported_flow.push_back(SERVER_SUPPORTED_FLOW{
                        idx_global_demand[k],
                        max_server_support_flow,
                        server_id});
                }
                //按照最大流量排序
                std::sort(server_supported_flow.begin(), server_supported_flow.end(),
                          [](const SERVER_SUPPORTED_FLOW &a, const SERVER_SUPPORTED_FLOW &b)
                          {
                              return a.max_flow > b.max_flow;
                          });

                server_supported_flow_2_time_vec.push_back(server_supported_flow);
            }

            //对server_supported_flow_2_time_vec按流量从大到小排序
            std::sort(server_supported_flow_2_time_vec.begin(), server_supported_flow_2_time_vec.end(),
                      [](const vector<SERVER_SUPPORTED_FLOW> &a, const vector<SERVER_SUPPORTED_FLOW> &b)
                      {
                          return a[0].max_flow > b[0].max_flow;
                      });

            return;
        }
    };

} // namespace solve
