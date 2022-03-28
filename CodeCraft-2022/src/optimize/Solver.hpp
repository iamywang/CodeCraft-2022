#pragma once

#include <iostream>
#include <functional>
#include <numeric>
#include <algorithm>
#include <queue>
#include "optimize_interface.hpp"
#include "../utils/tools.hpp"
#include "Optimizer.hpp"
#include "../utils/Verifier.hpp"

using namespace std;

namespace optimize
{
    typedef struct _MAX_5_PERCENT_FLOW
    {
        // max priority queue
        priority_queue<int, vector<int>, greater<int>> max_flow_queue;
    } MAX_5_PERCENT_FLOW;

    class Solver
    {
    private:
        vector<MAX_5_PERCENT_FLOW> m_max_5_percent_flow_vec; //边缘节点对应的能存储的最大的5%流量

        vector<vector<SERVER_SUPPORTED_FLOW>> m_server_supported_flow_2_time_vec;
        DEMAND m_demand;

    public:
        std::vector<ANSWER> &m_X_results;

    public:
        Solver(std::vector<ANSWER> &X_results, const DEMAND &demand) : m_X_results(X_results), m_demand(demand)
        {
        }
        ~Solver() {}

        int solve(const int num_iteration, const bool is_generate_initial_results)
        {
            sort_by_demand_and_qos(m_server_supported_flow_2_time_vec);
            if (is_generate_initial_results)
            {
                // vector<ANSWER> X_results;
                generate_initial_X_results();
            }

            for (auto &X : m_X_results)
            {
                for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
                {
                    int tmp = MyUtils::Tools::sum_column(X.flow, site_id);
                    X.sum_flow_site.push_back(tmp);
                    X.idx_local_mtime = this->m_demand.get(X.mtime);
                }
            }

            //根据时间对X_results进行排序
            std::sort(m_X_results.begin(), m_X_results.end(), [this](const ANSWER &a, const ANSWER &b)
                      { return a.idx_global_mtime < b.idx_global_mtime; });

            // //根据时间对server_supported_flow_2_time_vec排序
            std::sort(m_server_supported_flow_2_time_vec.begin(), m_server_supported_flow_2_time_vec.end(), [this](const vector<SERVER_SUPPORTED_FLOW> &a, const vector<SERVER_SUPPORTED_FLOW> &b)
                      { return a[0].idx_global_mtime < b[0].idx_global_mtime; });

            for (auto &v : m_server_supported_flow_2_time_vec)
            {
                //按照server_id排序
                std::sort(v.begin(), v.end(), [](const SERVER_SUPPORTED_FLOW &a, const SERVER_SUPPORTED_FLOW &b)
                          { return a.server_index < b.server_index; });
            }

            Optimizer(this->m_demand).optimize(m_server_supported_flow_2_time_vec, m_X_results, num_iteration);
#ifdef TEST
            if (Verifier(this->m_demand).verify(m_X_results))
            {
                cout << "verify success" << endl;
            }

            else
            {
                printf("verify failed\n");
                return -1;
            }
#endif
            return 0;
        }

    private:
        void generate_initial_X_results()
        {
            m_max_5_percent_flow_vec.clear();
            m_max_5_percent_flow_vec.resize(g_qos.site_name.size());

            m_X_results.resize(m_demand.mtime.size());
            // 求解每一时刻的X
            for (auto &server_supported_flow : m_server_supported_flow_2_time_vec)
            {
                const auto &mtime = server_supported_flow[0].mtime;
                const int index = server_supported_flow[0].idx_local_mtime;

#ifdef TEST
                if (index == -1)
                {
                    char buf[1024];
                    sprintf(buf, "mtime: %s not found in demand", mtime.c_str());
                    throw runtime_error(string(buf));
                }
#endif

                //取出对应时刻的demand
                vector<int> &demand_at_mtime = m_demand.demand[index];

                ANSWER &ans = m_X_results[index];
                ans.idx_global_mtime = m_demand.get_global_index(mtime);
                ans.idx_local_mtime = index;
                ans.mtime = mtime;
                solve_X(demand_at_mtime, g_site_bandwidth.bandwidth, g_qos.qos, server_supported_flow, ans);
            }
        }

        /**
         * @brief
         *
         * @param [out] server_supported_flow 边缘节点在各个时刻可提供的最大流量。每一行都是每个时刻各个边缘节点可以为当前所有客户端提供的最大流量。
         */
        void
        sort_by_demand_and_qos(vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_time_vec)
        {

            for (int k = 0; k < m_demand.demand.size(); k++)
            {
                vector<SERVER_SUPPORTED_FLOW> server_supported_flow;

                const auto &line_demand = m_demand.demand[k];

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
                        m_demand.get_global_index(m_demand.mtime[k]),
                        k,
                        m_demand.mtime[k],
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

            //对server_supported_flow_2_time_vec按从大到小排序
            std::sort(server_supported_flow_2_time_vec.begin(), server_supported_flow_2_time_vec.end(),
                      [](const vector<SERVER_SUPPORTED_FLOW> &a, const vector<SERVER_SUPPORTED_FLOW> &b)
                      {
                          return a[0].max_flow > b[0].max_flow;
                      });

            //打印server_supported_flow_2_time_vec
            // for(const auto& vec: server_supported_flow_2_time_vec)
            // {
            //     cout << vec[0].mtime << ": \n";
            //     for(const auto& flow: vec)
            //     {
            //         cout << flow.max_flow << " " << flow.server_index << endl;
            //     }
            //     cout << "\n\n\n";
            // }

            return;
        }

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
            //初始化X
            X.resize(demand.size());
            for (int i = 0; i < demand.size(); i++)
            {
                X[i] = vector<int>(g_qos.site_name.size(), 0); //这里必须初始化为0
            }

            const static int max_5_percent_flow_size = demand.size() - (int)((demand.size() - 1) * 0.95 + 1) + 1;

            vector<int> flow_server_left(server_supported_flow.size(), 0); //记录每个边缘节点还剩下多少流量可以恩培
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
            return;
        }
    };

} // namespace optimize
