#pragma once

#include "optimize_interface.hpp"
#include "../utils/tools.hpp"
#include <queue>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include "../utils/ProcessTimer.hpp"
#include "../utils/utils.hpp"
#include "./optimize_utils.hpp"

using namespace std;

namespace stream_optimize
{

    class Dispather
    {
    private:
        // const DEMAND &m_demand;
        const std::vector<int> &m_idx_global_demand;

        vector<vector<SERVER_FLOW>> &m_cost_vec_poll; //各行是对应的边缘节点在各个时刻的总的分配流量，保证各行是按照时间排序的
        vector<vector<SERVER_FLOW *>> &m_cost_vec;    //各行是对应的边缘节点在各个时刻的总的分配流量，但是并不保证每一行都是按照时间排序的

        const vector<vector<SERVER_SUPPORTED_FLOW>> &m_server_supported_flow_2_site_id_vec;
        vector<ANSWER> &m_X_results;

    public:
        Dispather(
            //   const DEMAND &demand,
            const std::vector<int> &idx_global_demand,
            vector<vector<SERVER_FLOW>> &cost_vec_poll_,
            vector<vector<SERVER_FLOW *>> &cost_vec_,
            const vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_site_id_vec_,
            vector<ANSWER> &X_results_) : // m_demand(demand),
                                          m_idx_global_demand(idx_global_demand),
                                          m_cost_vec_poll(cost_vec_poll_),
                                          m_cost_vec(cost_vec_),
                                          m_server_supported_flow_2_site_id_vec(server_supported_flow_2_site_id_vec_),
                                          m_X_results(X_results_)
        {
        }
        ~Dispather() {}

    private:
        int adjust_flow_up_limit(const vector<int> &flows_vec_95_according_site_id,
                                 const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow,
                                 const ANSWER &X,
                                 const SERVER_FLOW &quantile_server_flow,
                                 const int id_server_to_add,
                                 const double factor)
        {
            int added = 0;
            if (X.sum_flow_site[id_server_to_add] > flows_vec_95_according_site_id[id_server_to_add]) //当前节点的流量大于95%分位，那么可以肆无忌惮地增加
            {
                added = server_supported_flow[id_server_to_add].max_flow - X.sum_flow_site[id_server_to_add]; //该边缘节点允许增加的流量，由于本身是在后5%，因此可以让该边缘节点跑满
            }
            else //否则不能超过95%分位的限制
            {
                added = std::min(server_supported_flow[id_server_to_add].max_flow,
                                 int(flows_vec_95_according_site_id[id_server_to_add] * factor)) -
                        X.sum_flow_site[id_server_to_add];
            }

            added = std::min(added, *quantile_server_flow.flow); //这个是实际上可分配的流量
            return added;
        }

        bool update_X(ANSWER &X,
                      const vector<int> &flows_vec_95_according_site_id,
                      const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow,
                      const SERVER_FLOW &quantile_server_flow,
                      const double factor)
        {
            bool ret = false;

            //在不改变其他节点95%分位值的前提下,将当前流量分给其他的节点
            for (int server_id = 0;
                 server_id < g_site_bandwidth.site_name.size() && X.sum_flow_site[quantile_server_flow.site_id];
                 server_id++)
            {
                if (server_id == quantile_server_flow.site_id                                  //这个是显然的
                                                                                               // || X.sum_flow_site[server_id] == cost_vec_95_according_site_id[server_id] //如果这个节点的流量已经是95%分位了，就不能增加当前节点的流量了。可以增加，因为当前流量可能小于最小成本
                    || server_supported_flow[server_id].max_flow == X.sum_flow_site[server_id] //如果这个节点没有可供分配的流量了，就不能增加当前节点的流量了
                )
                {
                    continue;
                }

                int added = 0; //表示可以调整的流量上限

// #define USE_MATH_ANSLYSE
#ifdef USE_MATH_ANSLYSE

                {
                    int id1 = server_id;
                    int id2 = quantile_server_flow.site_id;

                    if (g_site_bandwidth.bandwidth[server_id] > g_site_bandwidth.bandwidth[quantile_server_flow.site_id])
                    {
                        std::swap(id1, id2);
                    }

                    int c1 = g_site_bandwidth.bandwidth[id1];
                    int c2 = g_site_bandwidth.bandwidth[id2];
                    int w1 = X.sum_flow_site[id1];
                    int w2 = X.sum_flow_site[id2];
                    int new_w1 = 0, new_w2 = 0;

                    if (w1 > g_minimum_cost)
                    {
                        if (w2 > g_minimum_cost)
                        {
                            new_w1 = std::min(w1 + w2, c1);
                            new_w2 = w1 + w2 - new_w1;
                            if (new_w2 > 0 && new_w2 < g_minimum_cost)
                            {
                                new_w2 = g_minimum_cost;
                                new_w1 = w1 + w2 - new_w2;
                            }
                        }
                        else
                        {
                            continue;
                        }
                    }
                    else
                    {
                        if (w1 + w2 > g_minimum_cost)
                        {
                            new_w1 = std::min(w1 + w2, c1);
                            new_w2 = w1 + w2 - new_w1;
                            if (new_w2)
                            {
                                // TODO 不知道哪个更好一些
                                //  new_w2 = g_minimum_cost;
                                //  new_w1 = w1 + w2 - new_w2;

                                new_w1 = g_minimum_cost;
                                new_w2 = w1 + w2 - new_w2;
                            }
                        }
                        else
                        {
                            new_w1 = w1 + w2;
                            new_w2 = 0;
                        }
                    }

                    if (id1 == server_id)
                    {
                        added = new_w2 - w2;
                    }
                    else
                    {
                        added = new_w1 - w1;
                    }
                }

                added = std::max(added, this->adjust_flow_up_limit(flows_vec_95_according_site_id,
                                                                   server_supported_flow,
                                                                   X,
                                                                   quantile_server_flow,
                                                                   server_id,
                                                                   factor));
#else
                added = this->adjust_flow_up_limit(flows_vec_95_according_site_id,
                                                   server_supported_flow,
                                                   X,
                                                   quantile_server_flow,
                                                   server_id,
                                                   factor);
#endif
#undef USE_MATH_ANSLYSE

                // 这里确定好了最多能分配给该边缘节点最多多少流量，即added
                while (added > 0)
                {
                    int most_match_stream_id = -1;
                    int most_match_client_id = -1;
                    double match_percent = 0;
                    int match_demand = 0;

                    const int size_of_stream2server_id = X.stream2server_id.size();
                    // 挑选占added比例最大的带宽需求进行调整
                    for (int stream_id = 0; stream_id < size_of_stream2server_id; stream_id++)
                    {
                        for (int client_id = 0; client_id < g_num_client; client_id++)
                        {
                            if (X.stream2server_id[stream_id][client_id] == quantile_server_flow.site_id)
                            {
                                if (g_qos.qos[server_id][client_id] == 0)
                                {
                                    continue;
                                }

                                // 计算占比最大的需求
                                const int current_demand = global::g_demand.stream_client_demand[X.idx_global_mtime].stream_2_client_demand[stream_id][client_id];
                                if (current_demand <= added)
                                {
                                    if (match_percent < (double)current_demand / added)
                                    {
                                        most_match_stream_id = stream_id;
                                        most_match_client_id = client_id;
                                        match_percent = (double)current_demand / added;
                                        match_demand = current_demand;
                                    }
                                }
                            }
                        }
                    }

                    // 没有分配到，跳出while循环
                    if (match_demand == 0)
                    {
                        added = 0;
                        break;
                    }

                    // 可以分配流量，调整分配方案
                    else
                    {
                        ret = true;
                        added -= match_demand;

                        // 调整流量-客户端分配矩阵
                        X.stream2server_id[most_match_stream_id][most_match_client_id] = server_id;

                        // 更新被调整服务器的流量
                        X.sum_flow_site[server_id] += match_demand;
                        X.sum_flow_site[quantile_server_flow.site_id] -= match_demand;

                        X.flow[most_match_client_id][server_id] += match_demand;
                        X.flow[most_match_client_id][quantile_server_flow.site_id] -= match_demand;

                        // 更新被调整服务器的成本
                        X.cost[server_id] = ANSWER::calculate_cost(server_id, X.sum_flow_site[server_id]);
                        X.cost[quantile_server_flow.site_id] = ANSWER::calculate_cost(quantile_server_flow.site_id,
                                                                                      X.sum_flow_site[quantile_server_flow.site_id]);
                    }
                }
            }
            return ret;
        }

    public:
        /**
         * @brief 通过将后5%的数据缩小使得95%分位值变小
         *
         * @param server_supported_flow_2_site_id_vec
         * @param quantile 调整第几分位值（对应的索引值）
         * @param m_cost_vec
         * @param X_results
         * @return true
         * @return false
         */
        bool dispath2(const int quantile)
        {
            bool ret = false;

            vector<SERVER_FLOW *> cost_vec_quantile;
            vector<int> flows_vec_quantile_according_site_id;
            get_server_flow_vec_by_quantile(quantile, m_cost_vec, cost_vec_quantile, flows_vec_quantile_according_site_id);

            vector<int> flows_vec_95_according_site_id(flows_vec_quantile_according_site_id.size(), 0);
            {
                int idx = calculate_quantile_index(0.95, this->m_idx_global_demand.size());
                if (idx == quantile)
                {
                    flows_vec_95_according_site_id = flows_vec_quantile_according_site_id;
                }
                else
                {
                    vector<SERVER_FLOW *> flows_vec_quantile2;
                    get_server_flow_vec_by_quantile(calculate_quantile_index(0.95, this->m_idx_global_demand.size()),
                                                    m_cost_vec, flows_vec_quantile2, flows_vec_95_according_site_id);
                }
            }

            {
                // int sum = std::accumulate(cost_vec_95_according_site_id.begin(), cost_vec_95_according_site_id.end(), 0);
                // printf("%d\n", sum);
            }

#ifdef SET_LOW_LIMIT
            vector<int> flows_vec_low_limit_according_site_id;

            {
                vector<SERVER_FLOW> tmp;
                get_server_flow_vec_by_quantile(calculate_quantile_index(0.8, this->m_idx_global_demand.size()),
                                                m_cost_vec,
                                                tmp,
                                                flows_vec_low_limit_according_site_id);
            }
#endif

            for (int id_min_quantile = 0; id_min_quantile < cost_vec_quantile.size(); id_min_quantile++)
            // for (int id_min_quantile = cost_vec_quantile.size() - 1; id_min_quantile >= 0; id_min_quantile--)//无法进行优化
            {
                //*
                SERVER_FLOW &min_quantile_server_flow = *cost_vec_quantile[id_min_quantile];
                ANSWER &X = m_X_results[min_quantile_server_flow.ans_id];
                const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow = m_server_supported_flow_2_site_id_vec[min_quantile_server_flow.ans_id];

                ret |= update_X(X,
                                flows_vec_95_according_site_id,
                                server_supported_flow,
                                min_quantile_server_flow,
                                0.4);
            }
            return ret;
        }

        bool dispath(const int max_95_percent_index)
        {
            bool ret = false;

            vector<SERVER_FLOW *> cost_vec_95_percent;
            vector<int> flows_vec_95_according_site_id;
            get_server_flow_vec_by_quantile(max_95_percent_index,
                                            m_cost_vec,
                                            cost_vec_95_percent,
                                            flows_vec_95_according_site_id);

            {
#ifdef TEST //TODO TEST
                double price_sum(0);
                for (const auto &item : cost_vec_95_percent)
                {
                    price_sum += *item->cost;
                }
                printf("price_sum: %f\n", price_sum);
                int sum = std::accumulate(flows_vec_95_according_site_id.begin(), flows_vec_95_according_site_id.end(), 0);
                printf("flows: %d\n", sum);
#endif
            }

            for (int id_max_95 = cost_vec_95_percent.size() - 1; id_max_95 >= 0; id_max_95--)
            {
                //*
                SERVER_FLOW &max_95_server_flow = *cost_vec_95_percent[id_max_95];
                ANSWER &X = m_X_results[max_95_server_flow.ans_id];
                const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow = m_server_supported_flow_2_site_id_vec[max_95_server_flow.ans_id];

                ret = update_X(X,
                               flows_vec_95_according_site_id,
                               // flows_vec_low_limit_according_site_id,
                               server_supported_flow,
                               max_95_server_flow,
                               0.8);
            }

            return false;
        }
    };

} // namespace optimize
