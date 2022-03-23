#include "optimize_interface.hpp"
#include "../utils/tools.hpp"
#include <queue>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include "../utils/ProcessTimer.hpp"
#include "../utils/utils.hpp"
using namespace std;

// #define SET_LOW_LIMIT
namespace optimize
{
    typedef struct _SERVER_FLOW
    {
        int ans_id;  //记录对应流量所在解的位置
        int site_id; //记录对应流量的边缘节点
        int flow;    //当前流量
    } SERVER_FLOW;

    extern void get_server_flow_vec_by_quantile(const int quantile,
                                                vector<vector<SERVER_FLOW>> &flows_vec,
                                                vector<SERVER_FLOW> &flows_vec_quantile,
                                                vector<int> &flows_vec_quantile_according_site_id);

    bool update_X(ANSWER &X,
                  const vector<int> &flows_vec_95_according_site_id,
                  const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow,
                  SERVER_FLOW &quantile_server_flow)
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
                                 int(flows_vec_95_according_site_id[server_id] * 0.4)) -
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

                if (added > 0) //有剩下未分配的，需要进行恢复
                {
                    X.sum_flow_site[server_id] -= added;
                    X.sum_flow_site[quantile_server_flow.site_id] += added;
                    // quantile_server_flow.flow += added;
                }
            }
        }
        return ret;
    }

    bool update_X2(ANSWER &X,
                   const vector<int> &flows_vec_95_according_site_id,
                   const vector<int> &flows_vec_low_limit_according_site_id,
                   const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow,
                   SERVER_FLOW &quantile_server_flow)
    {
        bool ret = false;

        //在不改变其他节点95%分位值的前提下,将当前流量分给其他的节点
        for (int server_id = 0;
             server_id < g_site_bandwidth.site_name.size();
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
                                 flows_vec_95_according_site_id[server_id] - 1) -
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
                for (int client_id = 0;
                     client_id < g_qos.client_name.size() && added && X.sum_flow_site[quantile_server_flow.site_id] > flows_vec_low_limit_according_site_id[quantile_server_flow.site_id];
                     client_id++)
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

                if (added > 0) //有剩下未分配的，需要进行恢复
                {
                    X.sum_flow_site[server_id] -= added;
                    X.sum_flow_site[quantile_server_flow.site_id] += added;
                    // quantile_server_flow.flow += added;
                }
            }

            if (X.sum_flow_site[quantile_server_flow.site_id] > flows_vec_low_limit_according_site_id[quantile_server_flow.site_id])
            {
                break;
            }
        }
        return ret;
    }

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
    bool dispath2(const vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_site_id_vec,
                  const int quantile,
                  vector<vector<SERVER_FLOW>> &flows_vec,
                  vector<ANSWER> &X_results)
    {
        bool ret = false;

        vector<SERVER_FLOW> flows_vec_quantile;
        vector<int> flows_vec_quantile_according_site_id;
        get_server_flow_vec_by_quantile(quantile, flows_vec, flows_vec_quantile, flows_vec_quantile_according_site_id);

        vector<int> flows_vec_95_according_site_id(flows_vec_quantile_according_site_id.size(), 0);
        {
            int idx = calculate_quantile_index(0.95);
            if (idx == quantile)
            {
                flows_vec_95_according_site_id = flows_vec_quantile_according_site_id;
            }
            else
            {
                vector<SERVER_FLOW> flows_vec_quantile2;
                get_server_flow_vec_by_quantile(calculate_quantile_index(0.95),
                                                flows_vec, flows_vec_quantile2, flows_vec_95_according_site_id);
            }
        }

        {
            // int sum = std::accumulate(flows_vec_95_according_site_id.begin(), flows_vec_95_according_site_id.end(), 0);
            // printf("%d\n", sum);
        }

#ifdef SET_LOW_LIMIT
        vector<int> flows_vec_low_limit_according_site_id;

        {
            vector<SERVER_FLOW> tmp;
            get_server_flow_vec_by_quantile(calculate_quantile_index(0.8),
                                            flows_vec,
                                            tmp,
                                            flows_vec_low_limit_according_site_id);
        }
#endif

        for (int id_min_quantile = 0; id_min_quantile < flows_vec_quantile.size(); id_min_quantile++)
        // for (int id_min_quantile = flows_vec_quantile.size() - 1; id_min_quantile >= 0; id_min_quantile--)//无法进行优化
        {
            //*
            SERVER_FLOW &min_quantile_server_flow = flows_vec_quantile[id_min_quantile];
            ANSWER &X = X_results[min_quantile_server_flow.ans_id];
            const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow = server_supported_flow_2_site_id_vec[min_quantile_server_flow.ans_id];

#ifdef SET_LOW_LIMIT
            ret = update_X2(X,
                            flows_vec_95_according_site_id,
                            flows_vec_low_limit_according_site_id,
                            server_supported_flow,
                            min_quantile_server_flow);
#else
            ret = update_X(X, flows_vec_95_according_site_id, server_supported_flow, min_quantile_server_flow);
#endif
        }
        return ret;
    }

    bool dispath(const vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_site_id_vec,
                 const int max_95_percent_index,
                 vector<vector<SERVER_FLOW>> &flows_vec,
                 vector<ANSWER> &X_results)
    {
        bool ret = false;

        vector<SERVER_FLOW> flows_vec_95_percent;
        vector<int> flows_vec_95_according_site_id;
        get_server_flow_vec_by_quantile(max_95_percent_index,
                                        flows_vec,
                                        flows_vec_95_percent,
                                        flows_vec_95_according_site_id);

#ifdef SET_LOW_LIMIT
        vector<int> flows_vec_low_limit_according_site_id;

        {
            vector<SERVER_FLOW> tmp;
            get_server_flow_vec_by_quantile(calculate_quantile_index(0.2),
                                            flows_vec,
                                            tmp,
                                            flows_vec_low_limit_according_site_id);
        }
#endif

        {
#ifdef TEST
            int sum = std::accumulate(flows_vec_95_according_site_id.begin(), flows_vec_95_according_site_id.end(), 0);
            printf("%d\n", sum);
#endif
        }

        for (int id_max_95 = flows_vec_95_percent.size() - 1; id_max_95 >= 0; id_max_95--)
        {
            //*
            SERVER_FLOW &max_95_server_flow = flows_vec_95_percent[id_max_95];
            ANSWER &X = X_results[max_95_server_flow.ans_id];
            const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow = server_supported_flow_2_site_id_vec[max_95_server_flow.ans_id];

#ifdef SET_LOW_LIMIT
            ret = update_X2(X,
                            flows_vec_95_according_site_id,
                            flows_vec_low_limit_according_site_id,
                            server_supported_flow,
                            max_95_server_flow);
#else
            ret = update_X(X,
                           flows_vec_95_according_site_id,
                           // flows_vec_low_limit_according_site_id,
                           server_supported_flow,
                           max_95_server_flow);
#endif
        }

        return ret;
    }
} // namespace optimize