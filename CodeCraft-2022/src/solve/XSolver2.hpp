#pragma once
#include "ResultGenerator.hpp"

namespace solve
{
    class XSolverGreedyAlgorithm2 : ResultGenerator
    {
    private:
    public:
        XSolverGreedyAlgorithm2(CommonDataForResultGenrator &common_data) : ResultGenerator(common_data) {}
        ~XSolverGreedyAlgorithm2() {}

        void generate_initial_X_results()
        {
            m_max_5_percent_flow_vec.clear();
            m_max_5_percent_flow_vec.resize(g_num_server);

            m_X_results.resize(m_idx_global_demand.size());

            // 求解每一时刻的X
            for (int idx = 0; idx < m_server_supported_flow_2_time_vec.size(); idx++)
            {
                auto &server_supported_flow = m_server_supported_flow_2_time_vec[idx];

                //取出对应时刻的demand
                auto &demand_at_mtime = global::g_demand.stream_client_demand[server_supported_flow[0].idx_global_mtime];

                ANSWER &ans = m_X_results[idx];
                ans.idx_global_mtime = server_supported_flow[0].idx_global_mtime;

                {
                    const int stream_nums = global::g_demand.stream_client_demand[server_supported_flow[0].idx_global_mtime].id_local_stream_2_stream_name.size();
                    ans.init(stream_nums);
                }

                solve_X(demand_at_mtime,
                        g_site_bandwidth.bandwidth_sorted_by_bandwidth_ascending_order,
                        g_qos.qos,
                        ans);
            }
        }

    private:
        /**
         * @brief
         *
         * @param [in] demand
         * @param [in] bandwidth
         * @param [in] QOS
         * @param [out] X
         */
        void solve_X(const STREAM_CLIENT_DEMAND &demand,
                     vector<SITE_BANDWIDTH::_inner_data> bandwidth_sorted, // copy一份
                     const vector<vector<int>> &QOS,
                     ANSWER &ans //行代表客户，列表示边缘节点
        )
        {
            ans.init(demand.stream_2_client_demand.size());

            for (int id_stream = 0; id_stream < demand.id_local_stream_2_stream_name.size(); id_stream++)
            {
                //每一行是一个流的需求
                for (int id_client = 0; id_client < g_num_client; id_client++)
                {
                    if (demand.stream_2_client_demand[id_stream][id_client] == 0)
                    {
                        continue;
                    }
                    for (auto &b : bandwidth_sorted)
                    {
                        if (b.bandwidth < demand.stream_2_client_demand[id_stream][id_client] || g_qos.qos[b.id_server][id_client] == 0)
                        {
                            continue;
                        }
                        ans.set(id_stream, id_client, b.id_server);
                        b.bandwidth -= demand.stream_2_client_demand[id_stream][id_client];
                        
                        break;
                    }
#ifdef TEST
                    if (ans.stream2server_id[id_stream][id_client] == -1)
                    {
                        throw runtime_error("no enough bandwidth");
                        exit(-1);
                    }
#endif
                }
            }

            for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
            {
                int tmp = MyUtils::Tools::sum_column(ans.flow, site_id);
                ans.sum_flow_site[site_id] = (tmp);
                ans.cost[site_id] = (ANSWER::calculate_cost(site_id, tmp));
            }
            return;
        }
    };
} // namespace solve
