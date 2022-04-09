#pragma once
#include "ResultGenerator.hpp"
// #include "../utils/Graph/MaxFlow.hpp"
namespace solve
{
    class XSolverGreedyAlgorithm : ResultGenerator
    {
    private:
    public:
        XSolverGreedyAlgorithm(CommonDataForResultGenrator &common_data) : ResultGenerator(common_data) {}
        ~XSolverGreedyAlgorithm() {}

        /**
         * @brief 初始化解。会对ANSWER.flow, ANSWER.sum_flow_site, ANSWER.cost进行初始化。
         *
         */
        void generate_initial_X_results()
        {
            m_max_5_percent_flow_vec.clear();
            m_max_5_percent_flow_vec.resize(g_qos.site_name.size());

            // m_X_results.resize(m_demand.mtime.size());
            m_X_results.resize(m_idx_global_demand.size());

            // 求解每一时刻的X
            for (int idx = 0; idx < m_server_supported_flow_2_time_vec.size(); idx++)
            // for (auto &server_supported_flow : m_server_supported_flow_2_time_vec)
            {
                auto &server_supported_flow = m_server_supported_flow_2_time_vec[idx];

                //取出对应时刻的demand
                vector<int> &demand_at_mtime = global::g_demand.client_demand[server_supported_flow[0].idx_global_mtime];

                ANSWER &ans = m_X_results[idx];
                ans.idx_global_mtime = server_supported_flow[0].idx_global_mtime;

                {
                    const int stream_nums = global::g_demand.stream_client_demand[server_supported_flow[0].idx_global_mtime].id_local_stream_2_stream_name.size();
                    ans.init(stream_nums);
                }
                solve_X(demand_at_mtime, g_site_bandwidth.bandwidth, g_qos.qos, server_supported_flow, ans);
            }
            return;
        }

    private:
        int pick_server(const vector<SERVER_SUPPORTED_FLOW> &server_supported_flow,
                        int max_5_percent_flow_size)
        {
            int server_id = 0;

            int i = 0;
            for (; i < server_supported_flow.size(); i++)
            {
                if (m_max_5_percent_flow_vec[server_supported_flow[i].server_index].max_flow_queue.size() > max_5_percent_flow_size)
                {
                    continue;
                }
                if (server_supported_flow[i].max_flow == 0)
                {
                    break;
                }
                server_id = server_supported_flow[i].server_index;
                break;
            }

            return server_id;
        }

        /**
         * @brief
         *
         * @param [in] demand
         * @param [in] bandwidth
         * @param [in] QOS
         * @param [out] X
         */
        void solve_X(vector<int> demand, // copy一份
                     const vector<int> &bandwidth,
                     const vector<vector<int>> &QOS,
                     const vector<SERVER_SUPPORTED_FLOW> &server_supported_flow,
                     ANSWER &ans //行代表客户，列表示边缘节点
        )
        {

            auto &X = ans.flow;


            const static int max_5_percent_flow_size = demand.size() - (int)((demand.size() - 1) * 0.95 + 1) + 1;

            vector<int> flow_server_left(server_supported_flow.size(), 0); //记录每个边缘节点还剩下多少流量可以分配
            for (const auto &i : server_supported_flow)
            {
                flow_server_left[i.server_index] = i.max_flow;
            }

            {
                int site_index = pick_server(server_supported_flow, max_5_percent_flow_size);
                m_max_5_percent_flow_vec[site_index].max_flow_queue.push(server_supported_flow[site_index].max_flow);

                //尽量使得该边缘节点带宽跑满
                for (int clinet_id = 0; clinet_id < demand.size() && flow_server_left[site_index]; clinet_id++)
                {
                    if (g_qos.qos[site_index][clinet_id] == 0)
                        continue;

                    int tmp = std::min(std::min(flow_server_left[site_index], demand[clinet_id]),
                                       demand[clinet_id]);

                    X[clinet_id][site_index] += tmp;
                    flow_server_left[site_index] -= tmp;
                    demand[clinet_id] -= tmp;
                }
                // cout << endl;
            }

            {
                //分配剩余的客户流量

                for (int client_id = 0; client_id < demand.size(); client_id++)
                {

                    while (demand[client_id] != 0)
                    {
                        vector<int> server_id_vec; //记录可以用于分配的边缘节点
                        for (int i = 0; i < flow_server_left.size(); i++)
                        {
                            if (flow_server_left[i] > 0 && (g_qos.qos[i][client_id] > 0))
                            {
                                server_id_vec.push_back(i);
                            }
                        }
                        if (server_id_vec.size() == 0)
                        {
                            break;
                        }

                        int avg_flow_to_dispached = demand[client_id] / server_id_vec.size() + 1; //每个边缘节点平均可分配的流量
                        for (int i = 0; i < server_id_vec.size(); i++)
                        {
                            int server_id = server_id_vec[i];
                            int tmp = std::min(std::min(flow_server_left[server_id], demand[client_id]),
                                               avg_flow_to_dispached);

                            X[client_id][server_id] += tmp;
                            flow_server_left[server_id] -= tmp;
                            demand[client_id] -= tmp;
                        }
                    }
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

    // class XSolverMaxFlow : ResultGenerator
    // {
    // private:
    // public:
    //     XSolverMaxFlow(CommonDataForResultGenrator &common_data) : ResultGenerator(common_data) {}
    //     ~XSolverMaxFlow() {}
    //     void generate_initial_X_results()
    //     {
    //         // m_X_results.resize(m_demand.mtime.size());
    //         m_X_results.resize(m_idx_global_demand.size());

    //         // 求解每一时刻的X
    //         for (int index = 0; index < m_idx_global_demand.size(); index++)
    //         {
    //             const string &mtime = global::g_demand.mtime[m_idx_global_demand[index]];

    //             //取出对应时刻的demand
    //             vector<int> &demand_at_mtime = global::g_demand.client_demand[m_idx_global_demand[index]];

    //             ANSWER &ans = m_X_results[index];
    //             ans.idx_global_mtime = global::g_demand.get_global_index(mtime);
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

    //         {
    //             // verify
    //             //  int sum_dem = std::accumulate(demand.begin(), demand.end(), 0);
    //             //  if(sum_dem != max_flow)
    //             //  {
    //             //      cout << "sum_dem != max_flow, error" << endl;
    //             //      exit(-1);
    //             //  }
    //         }
    //         return;
    //     }
    // };

} // namespace solve
