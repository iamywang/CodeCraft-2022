#pragma once
#include "ResultGenerator.hpp"
#include "../utils/Graph/MaxFlow.hpp"
namespace solve
{
    class XSolverGreedyAlgorithm : ResultGenerator
    {
    private:
    public:
        XSolverGreedyAlgorithm(CommonDataForResultGenrator &common_data) : ResultGenerator(common_data) {}
        ~XSolverGreedyAlgorithm() {}

        void generate_initial_X_results()
        {
            m_max_5_percent_flow_vec.clear();
            m_max_5_percent_flow_vec.resize(g_num_server);

            m_X_results.resize(m_demand.mtime.size());
            // 求解每一时刻的X
            for (int index = 0; index < m_demand.mtime.size(); index++)
            {
                const auto mtime = m_demand.mtime[index];

                //取出对应时刻的demand
                auto &demand_at_mtime = m_demand.demand[index];

                ANSWER &ans = m_X_results[index];
                ans.idx_global_mtime = m_demand.get_global_index(mtime);
                ans.idx_local_mtime = index;
                ans.mtime = mtime;
                solve_X(demand_at_mtime, g_site_bandwidth.bandwidth_sorted_by_bandwidth_ascending_order, g_qos.qos, ans);
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
        void solve_X(const STREAM_CLIENT_DEMAND &demand,                   // copy一份
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
                    if(demand.stream_2_client_demand[id_stream][id_client] == 0)
                    {
                        continue;
                    }
                    for (auto &b : bandwidth_sorted)
                    {
                        if (b.bandwidth < demand.stream_2_client_demand[id_stream][id_client])
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

            return;
        }
    };

    // class XSolverMaxFlow : ResultGenerator
    // {
    // private:
    // public:
    //     XSolverMaxFlow(CommonDataForResultGenrator &common_data) : ResultGenerator(common_data) {}
    //     ~XSolverMaxFlow() {}
    //     void generate_initial_X_results()
    //     {
    //         m_X_results.resize(m_demand.mtime.size());
    //         // 求解每一时刻的X
    //         for (int index = 0; index < m_demand.mtime.size(); index++)
    //         {
    //             const string &mtime = m_demand.mtime[index];

    //             //取出对应时刻的demand
    //             vector<int> &demand_at_mtime = m_demand.demand[index];

    //             ANSWER &ans = m_X_results[index];
    //             ans.idx_global_mtime = m_demand.get_global_index(mtime);
    //             ans.idx_local_mtime = index;
    //             ans.mtime = mtime;
    //             solve_X(demand_at_mtime, g_site_bandwidth.bandwidth, g_qos.qos, ans);
    //         }
    //         return;
    //     }

    //     /**
    //      * @brief 使用最大流算法计算初始解
    //      *
    //      * @param [in] demand
    //      * @param [in] bandwidth
    //      * @param [in] QOS
    //      * @param [out] ans
    //      */
    //     static void solve_X(const vector<int> &demand,
    //                         const vector<int> &bandwidth,
    //                         const vector<vector<int>> &QOS,
    //                         ANSWER &ans //行代表客户，列表示边缘节点
    //     )
    //     {
    //         const int num_vertexes = g_qos.site_name.size() + g_qos.client_name.size() + 2;
    //         const int id_vertex_src = 0, id_vertex_dst = num_vertexes - 1;
    //         const int id_base_vertex_site = g_qos.client_name.size() + 1;

    //         Graph::Algorithm::MaxFlowGraph graph;
    //         graph.initGraph(num_vertexes);
    //         for (int client_id = 0; client_id < g_qos.client_name.size(); client_id++)
    //         {
    //             const int id_vertex_client = client_id + 1;

    //             graph.addEdge(id_vertex_src, id_vertex_client, demand[client_id]);

    //             for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
    //             {
    //                 if (QOS[site_id][client_id] > 0)
    //                 {
    //                     graph.addEdge(id_vertex_client, site_id + id_base_vertex_site, g_site_bandwidth.bandwidth[site_id]);
    //                 }
    //             }
    //         }

    //         for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
    //         {
    //             graph.addEdge(site_id + id_base_vertex_site, id_vertex_dst, g_site_bandwidth.bandwidth[site_id]);
    //         }

    //         int max_flow = graph.getMaxFlow(id_vertex_src, id_vertex_dst);

    //         {
    //             auto &edges = graph.getEdges();
    //             auto &X = ans.flow;
    //             //初始化X
    //             X.resize(demand.size());
    //             for (int i = 0; i < demand.size(); i++)
    //             {
    //                 X[i] = vector<int>(g_qos.site_name.size(), 0); //这里必须初始化为0
    //             }
    //             for (auto &edge : edges)
    //             {
    //                 if (edge.size() > 0 && edge[0].fromVertex > id_vertex_src && edge[0].toVertex < id_vertex_dst)
    //                     for (auto &e : edge)
    //                     {
    //                         X[e.fromVertex - 1][e.toVertex - id_base_vertex_site] = e.flow;
    //                     }
    //             }
    //         }
    //         for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
    //         {
    //             int tmp = MyUtils::Tools::sum_column(ans.flow, site_id);
    //             ans.sum_flow_site.push_back(tmp);
    //         }
    //         return;
    //     }
    // };

} // namespace solve
