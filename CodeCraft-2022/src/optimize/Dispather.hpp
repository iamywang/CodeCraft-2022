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

namespace optimize
{

    class Dispather
    {
    private:
        // const DEMAND &m_demand;
        const std::vector<int> &m_idx_global_demand;

        vector<vector<SERVER_FLOW>> &m_flows_vec_poll; //各行是对应的边缘节点在各个时刻的总的分配流量，保证各行是按照时间排序的
        vector<vector<SERVER_FLOW *>> &m_flows_vec;    //各行是对应的边缘节点在各个时刻的总的分配流量，但是并不保证每一行都是按照时间排序的

        const vector<vector<SERVER_SUPPORTED_FLOW>> &m_server_supported_flow_2_site_id_vec;
        vector<ANSWER> &m_X_results;

    public:
        Dispather(
            //   const DEMAND &demand,
            const std::vector<int> &idx_global_demand,
            vector<vector<SERVER_FLOW>> &flows_vec_poll_,
            vector<vector<SERVER_FLOW *>> &flows_vec_,
            const vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_site_id_vec_,
            vector<ANSWER> &X_results_) : // m_demand(demand),
                                          m_idx_global_demand(idx_global_demand),
                                          m_flows_vec_poll(flows_vec_poll_),
                                          m_flows_vec(flows_vec_),
                                          m_server_supported_flow_2_site_id_vec(server_supported_flow_2_site_id_vec_),
                                          m_X_results(X_results_)
        {
        }
        ~Dispather() {}

    private:
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
                    || X.sum_flow_site[server_id] == flows_vec_95_according_site_id[server_id] //如果这个节点的流量已经是95%分位了，就不能增加当前节点的流量了
                    || server_supported_flow[server_id].max_flow == X.sum_flow_site[server_id] //如果这个节点没有可供分配的流量了，就不能增加当前节点的流量了
                )
                {
                    continue;
                }
                //这里找到了可以用来调整的边缘节点
                int added = 0;                                                              //表示可以调整的流量上限
                if (X.sum_flow_site[server_id] > flows_vec_95_according_site_id[server_id]) //当前节点的流量大于95%分位，那么可以肆无忌惮地增加
                {
                    added = server_supported_flow[server_id].max_flow - X.sum_flow_site[server_id]; //该边缘节点允许增加的流量，由于本身是在后5%，因此可以让该边缘节点跑满
                }
                else //否则不能超过95%分位的限制
                {
                    added = std::min(server_supported_flow[server_id].max_flow,
                                     int(flows_vec_95_according_site_id[server_id] * factor)) -
                            X.sum_flow_site[server_id];
                }

                added = std::min(added, quantile_server_flow.flow); //这个是实际上可分配的流量

                //这里确定好了最多能分配给该边缘节点最多多少流量，即added
                if (added > 0)
                {
                    //在做下述调整的时候，flows_vec相关的内容不需要调整，需要对X的相关值做调整，需要对flows_vec_95_percent_according_site_id做调整
                    //对flows_vec_95_percent_according_site_id而言，因为被加上的边缘节点的95%分位流量不会改变，只需要调整max_95_server_flow即可
                    X.sum_flow_site[server_id] += added;
                    X.sum_flow_site[quantile_server_flow.site_id] -= added;
                    // quantile_server_flow.flow -= added; //这里究竟要不要调整呢？是个问题。如果调整的话，可能会出现95%分位值被设置过小的情况

                    //调整分配方案
                    for (int client_id = 0; client_id < g_qos.client_name.size() && added; client_id++)
                    {
                        if (g_qos.qos[server_id][client_id] > 0)
                        {
                            ret = true;
                            if (X.flow[client_id][quantile_server_flow.site_id] >= added)
                            {
                                X.flow[client_id][server_id] += added;
                                X.flow[client_id][quantile_server_flow.site_id] -= added;
                                added = 0;
                                break;
                            }
                            else // added比较大
                            {
                                if (X.flow[client_id][quantile_server_flow.site_id])
                                {
                                    added -= X.flow[client_id][quantile_server_flow.site_id];
                                    X.flow[client_id][server_id] += X.flow[client_id][quantile_server_flow.site_id];
                                    X.flow[client_id][quantile_server_flow.site_id] = 0;
                                }
                            }
                        }
                    }

                    X.sum_flow_site[server_id] -= added;
                    m_flows_vec_poll[server_id][quantile_server_flow.ans_id].flow = X.sum_flow_site[server_id];
                    X.cost[server_id] = ANSWER::calculate_cost(server_id, X.sum_flow_site[server_id]);
                    m_flows_vec_poll[server_id][quantile_server_flow.ans_id].cost = X.cost[server_id];

                    X.sum_flow_site[quantile_server_flow.site_id] += added;
                    m_flows_vec_poll[quantile_server_flow.site_id][quantile_server_flow.ans_id].flow = X.sum_flow_site[quantile_server_flow.site_id];
                    X.cost[quantile_server_flow.site_id] = ANSWER::calculate_cost(quantile_server_flow.site_id, X.sum_flow_site[quantile_server_flow.site_id]);
                    m_flows_vec_poll[quantile_server_flow.site_id][quantile_server_flow.ans_id].cost = X.cost[quantile_server_flow.site_id];
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
         * @param flows_vec
         * @param X_results
         * @return true
         * @return false
         */
        bool dispath2(const double quantile)
        {
            bool ret = false;

            vector<SERVER_FLOW *> flows_vec_quantile;
            vector<int> flows_vec_quantile_according_site_id;
            get_server_flow_vec_by_quantile(quantile, m_flows_vec, flows_vec_quantile, flows_vec_quantile_according_site_id);

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
                    get_server_flow_vec_by_quantile(0.95,
                                                    m_flows_vec,
                                                    flows_vec_quantile2,
                                                    flows_vec_95_according_site_id);
                }
            }

            {
                // int sum = std::accumulate(flows_vec_95_according_site_id.begin(), flows_vec_95_according_site_id.end(), 0);
                // printf("%d\n", sum);
            }

            for (int id_min_quantile = 0; id_min_quantile < flows_vec_quantile.size(); id_min_quantile++)
            // for (int id_min_quantile = flows_vec_quantile.size() - 1; id_min_quantile >= 0; id_min_quantile--)//无法进行优化
            {
                //*
                SERVER_FLOW &min_quantile_server_flow = *flows_vec_quantile[id_min_quantile];
                ANSWER &X = m_X_results[min_quantile_server_flow.ans_id];
                const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow = m_server_supported_flow_2_site_id_vec[min_quantile_server_flow.ans_id];

                ret |= update_X(X,
                                flows_vec_95_according_site_id,
                                server_supported_flow,
                                min_quantile_server_flow,
                                0.7);
            }
            // if(ret == true)
            // {
            //     throw runtime_error("ret == true");
            //     cout << __func__ << ": ret = true" << endl;
            //     exit(-1);
            // }
            return ret;
        }

        bool dispath()
        {

            vector<SERVER_FLOW *> flows_vec_95_percent;
            vector<int> flows_vec_95_according_site_id;
            get_server_flow_vec_by_quantile(0.95,
                                            m_flows_vec,
                                            flows_vec_95_percent,
                                            flows_vec_95_according_site_id);

            {
                #ifdef TEST //TODO TEST
                int sum = std::accumulate(flows_vec_95_according_site_id.begin(), flows_vec_95_according_site_id.end(), 0);
                printf("%d\n", sum);
                #endif
            }

            bool ret = false;
            // for (int i = 0; i < 1; ++i)
            {
                for (int id_max_95 = flows_vec_95_percent.size() - 1; id_max_95 >= 0; id_max_95--)
                {
                    //*
                    SERVER_FLOW &max_95_server_flow = *flows_vec_95_percent[id_max_95];
                    ANSWER &X = m_X_results[max_95_server_flow.ans_id];
                    const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow = m_server_supported_flow_2_site_id_vec[max_95_server_flow.ans_id];

                    ret = update_X(X,
                                   flows_vec_95_according_site_id,
                                   // flows_vec_low_limit_according_site_id,
                                   server_supported_flow,
                                   max_95_server_flow,
                                   0.85);
                }
            }

            // if(ret == true)
            // {
            //     throw runtime_error("ret == true");
            //     cout << __func__ << ": ret = true" << endl;
            //     exit(-1);
            // }

            return false;
        }
    };

} // namespace optimize
