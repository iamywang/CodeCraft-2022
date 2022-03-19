#include "global.hpp"
#include "tools.hpp"
#include <queue>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
using namespace std;

typedef struct _SERVER_FLOW
{
    int ans_id;  //记录对应流量所在解的位置
    int site_id; //记录对应流量的边缘节点
    int flow;    //当前流量
} SERVER_FLOW;

/**
 * @brief 根据size计算对应百分位值的索引（向上取整）
 *
 * @param quantile
 * @param size
 * @return int
 */
int calculate_quantile_index(double quantile, int size)
{
    return std::ceil(quantile * size) - 1; //因为是索引，所以需要减去1
}

/**
 * @brief 函数会对flows_vec按照flow从小到大排序
 *
 * @param quantile 百分位数对应索引值
 * @param [in|out] flows_vec 会对flows_vec按照flow从小到大排序
 * @param [out] flows_vec_quantile_vec 只记录第quantile百分位数的的SERVER_FLOW，根据flow从小到大排序
 * @param [out] flows_vec_quantile_vec_idx 按照site_id进行索引的第quantile的流量值
 */
void get_server_flow_vec_by_quantile(const int quantile,
                                     vector<vector<SERVER_FLOW>> &flows_vec,
                                     vector<SERVER_FLOW> &flows_vec_quantile,
                                     vector<int> &flows_vec_quantile_according_site_id)
{
    for (auto &v : flows_vec)
    {
        //根据流量从小到大排序
        std::sort(v.begin(), v.end(), [](const SERVER_FLOW &a, const SERVER_FLOW &b)
                  { return a.flow < b.flow; });
    }

    { //提取quantile百分位数的SERVER_FLOW
        for (int i = 0; i < flows_vec.size(); i++)
        {
            flows_vec_quantile.push_back(flows_vec[i][quantile]);
        }
        //对flows_vec_quantile从小到大排序
        std::sort(flows_vec_quantile.begin(), flows_vec_quantile.end(),
                  [](const SERVER_FLOW &a, const SERVER_FLOW &b)
                  { return a.flow < b.flow; });
    }

    {
        flows_vec_quantile_according_site_id.resize(flows_vec_quantile.size());
        for (const auto &i : flows_vec_quantile)
        {
            flows_vec_quantile_according_site_id[i.site_id] = i.flow;
        }
    }
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
bool dispath2(const vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_site_id_vec, const int quantile, vector<vector<SERVER_FLOW>> &flows_vec, vector<ANSWER> &X_results)
{
    bool ret = false;

    vector<SERVER_FLOW> flows_vec_quantile;
    vector<int> flows_vec_quantile_according_site_id;
    get_server_flow_vec_by_quantile(quantile, flows_vec, flows_vec_quantile, flows_vec_quantile_according_site_id);

    vector<int> flows_vec_95_according_site_id(flows_vec_quantile_according_site_id.size(), 0);
    {
        int idx = calculate_quantile_index(0.95, flows_vec_quantile_according_site_id.size());
        if (idx == quantile)
        {
            flows_vec_95_according_site_id = flows_vec_quantile_according_site_id;
        }
        else
        {
            vector<SERVER_FLOW> flows_vec_quantile2;
            get_server_flow_vec_by_quantile(calculate_quantile_index(0.95, g_site_bandwidth.site_name.size()),
                                            flows_vec, flows_vec_quantile2, flows_vec_95_according_site_id);
        }
    }

    {
        // int sum = std::accumulate(flows_vec_95_according_site_id.begin(), flows_vec_95_according_site_id.end(), 0);
        // printf("%d\n", sum);
    }

    for (int id_min_quantile = 0; id_min_quantile < flows_vec_quantile.size(); id_min_quantile++)
    {
        //*
        SERVER_FLOW &min_quantile_server_flow = flows_vec_quantile[id_min_quantile];
        ANSWER &X = X_results[min_quantile_server_flow.ans_id];
        const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow = server_supported_flow_2_site_id_vec[min_quantile_server_flow.ans_id];

        //在不改变其他节点95%分位值的前提下,将当前流量分给其他的节点
        for (int server_id = 0; server_id < g_site_bandwidth.site_name.size() && min_quantile_server_flow.flow; server_id++)
        {
            if (server_id == min_quantile_server_flow.site_id                                    //这个是显然的
                || X.sum_flow_site[server_id] == flows_vec_quantile_according_site_id[server_id] //如果这个节点的流量已经是95%分位了，就不能增加当前节点的流量了
            )
            {
                continue;
            }

            //这里找到了可以用来调整的边缘节点
            {
                int added = 0;
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

                added = std::min(added, min_quantile_server_flow.flow); //这个是实际上可分配的流量

                //这里确定好了最多能分配给该边缘节点最多多少流量，即added
                if (added > 0)
                {
                    //在做下述调整的时候，flows_vec相关的内容不需要调整，需要对X的相关值做调整，需要对flows_vec_95_percent_according_site_id做调整
                    //对flows_vec_95_percent_according_site_id而言，因为被加上的边缘节点的95%分位流量不会改变，只需要调整max_95_server_flow即可
                    X.sum_flow_site[server_id] += added;
                    X.sum_flow_site[min_quantile_server_flow.site_id] -= added;
                    min_quantile_server_flow.flow -= added;

                    //调整分配方案
                    for (int client_id = 0; client_id < g_qos.client_name.size() && added; client_id++)
                    {
                        if (g_qos.qos[server_id][client_id] > 0)
                        {
                            ret = true;
                            if (X.flow[client_id][min_quantile_server_flow.site_id] >= added)
                            {
                                X.flow[client_id][server_id] += added;
                                X.flow[client_id][min_quantile_server_flow.site_id] -= added;
                                break;
                            }
                            else // added比较大
                            {
                                added -= X.flow[client_id][min_quantile_server_flow.site_id];
                                X.flow[client_id][server_id] += X.flow[client_id][min_quantile_server_flow.site_id];
                                X.flow[client_id][min_quantile_server_flow.site_id] = 0;
                            }
                        }
                    }
                }
            }
        }
    }
    return ret;
}

bool dispath(const vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_site_id_vec, const int max_95_percent_index, vector<vector<SERVER_FLOW>> &flows_vec, vector<ANSWER> &X_results)
{
    bool ret = false;

    vector<SERVER_FLOW> flows_vec_95_percent;
    vector<int> flows_vec_95_percent_according_site_id;
    get_server_flow_vec_by_quantile(max_95_percent_index, flows_vec, flows_vec_95_percent, flows_vec_95_percent_according_site_id);

    {
        // int sum = std::accumulate(flows_vec_95_percent_according_site_id.begin(), flows_vec_95_percent_according_site_id.end(), 0);
        // printf("%d\n", sum);
    }

    for (int id_max_95 = flows_vec_95_percent.size() - 1; id_max_95 >= 0; id_max_95--)
    // for (int id_max_95 = flows_vec_95_percent.size() - 1; id_max_95 >= int(double(flows_vec_95_percent.size()) * 0.); id_max_95--)
    {
        //*
        SERVER_FLOW &max_95_server_flow = flows_vec_95_percent[id_max_95];
        ANSWER &X = X_results[max_95_server_flow.ans_id];
        const std::vector<SERVER_SUPPORTED_FLOW> &server_supported_flow = server_supported_flow_2_site_id_vec[max_95_server_flow.ans_id];

        //在不改变其他节点95%分位值的前提下,将当前流量分给其他的节点
        for (int server_id = 0; server_id < g_site_bandwidth.site_name.size() && max_95_server_flow.flow; server_id++)
        {
            if (server_id == max_95_server_flow.site_id                                            //这个是显然的
                || X.sum_flow_site[server_id] == flows_vec_95_percent_according_site_id[server_id] //如果这个节点的流量已经是95%分位了，就不能增加当前节点的流量了
            )
            {
                continue;
            }

            //这里找到了可以用来调整的边缘节点
            {
                int added = 0;
                if (X.sum_flow_site[server_id] > flows_vec_95_percent_according_site_id[server_id]) //当前节点的流量大于95%分位，那么可以肆无忌惮地增加
                {
                    added = server_supported_flow[server_id].max_flow - X.sum_flow_site[server_id]; //该边缘节点允许增加的流量，由于本身是在后5%，因此可以让该边缘节点跑满
                }
                else //否则不能超过95%分位的限制
                {
                    added = std::min(server_supported_flow[server_id].max_flow,
                                     flows_vec_95_percent_according_site_id[server_id] - 1) -
                            X.sum_flow_site[server_id];
                }

                added = std::min(added, max_95_server_flow.flow); //这个是实际上可分配的流量

                //这里确定好了最多能分配给该边缘节点最多多少流量，即added
                if (added > 0)
                {
                    //在做下述调整的时候，flows_vec相关的内容不需要调整，需要对X的相关值做调整，需要对flows_vec_95_percent_according_site_id做调整
                    //对flows_vec_95_percent_according_site_id而言，因为被加上的边缘节点的95%分位流量不会改变，只需要调整max_95_server_flow即可
                    X.sum_flow_site[server_id] += added;
                    X.sum_flow_site[max_95_server_flow.site_id] -= added;
                    max_95_server_flow.flow -= added;

                    //调整分配方案
                    for (int client_id = 0; client_id < g_qos.client_name.size() && added; client_id++)
                    {
                        if (g_qos.qos[server_id][client_id] > 0)
                        {
                            ret = true;
                            if (X.flow[client_id][max_95_server_flow.site_id] >= added)
                            {
                                X.flow[client_id][server_id] += added;
                                X.flow[client_id][max_95_server_flow.site_id] -= added;
                                break;
                            }
                            else // added比较大
                            {
                                added -= X.flow[client_id][max_95_server_flow.site_id];
                                X.flow[client_id][server_id] += X.flow[client_id][max_95_server_flow.site_id];
                                X.flow[client_id][max_95_server_flow.site_id] = 0;
                            }
                        }
                    }
                }
            }
        }
        //*/
    }

    return ret;
}

/**
 * @brief
 *
 * @param [in] server_supported_flow_2_site_id_vec该参数是已经按照时刻排好序了的。且每一行都是按照site_id排序。
 * @param [in|out] X_results 该参数是已经按照时刻排好序了的
 */
void optimize(const vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_site_id_vec, vector<ANSWER> &X_results)
{
    vector<vector<SERVER_FLOW>> flows_vec; //各行是对应的边缘节点在各个时刻的总的分配流量
    for (int i = 0; i < g_site_bandwidth.site_name.size(); i++)
    {
        flows_vec.push_back(vector<SERVER_FLOW>(g_demand.mtime.size()));
    }

    for (int id_ans; id_ans < X_results.size(); id_ans++)
    {
        auto &X = X_results[id_ans];
        X.sum_flow_site.resize(0);
        for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
        {
            int sum_flow = MyUtils::Tools::sum_column(X.flow, site_id);
            X.sum_flow_site.push_back(sum_flow);
        }
    }

    // const int max_95_percent_index = std::ceil(g_site_bandwidth.site_name.size() * 0.95) - 1; // 95%分位的索引需要减去1
    const int max_95_percent_index = calculate_quantile_index(0.95, g_site_bandwidth.site_name.size());

    // for (int i = 0; i < 1000; i++)
    // {
    //     int last_quantile_index = -1;
    //     bool flag = false;

    //     for (int j = 95; j <= 100; j++)
    //     {
    //         int quantile_index = calculate_quantile_index(double(j) / 100.0, g_site_bandwidth.site_name.size());
    //         if (quantile_index == last_quantile_index)
    //         {
    //             continue;
    //         }
    //         last_quantile_index = quantile_index;

    //         for (int id_ans = 0; id_ans < X_results.size(); id_ans++)
    //         {
    //             const auto &X = X_results[id_ans];
    //             for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
    //             {
    //                 flows_vec[site_id][id_ans] = SERVER_FLOW{id_ans, site_id, X.sum_flow_site[site_id]}; //因为是按照所有时刻计算的，所以即使为0也要计算在内
    //             }
    //         }

    //         bool flag_tmp = dispath2(server_supported_flow_2_site_id_vec, quantile_index, flows_vec, X_results);
    //         flag = flag || flag_tmp;
    //         if (flag_tmp)
    //         {
    //             for (int id_ans = 0; id_ans < X_results.size(); id_ans++)
    //             {
    //                 const auto &X = X_results[id_ans];
    //                 for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
    //                 {
    //                     flows_vec[site_id][id_ans] = SERVER_FLOW{id_ans, site_id, X.sum_flow_site[site_id]}; //因为是按照所有时刻计算的，所以即使为0也要计算在内
    //                 }
    //             }
    //         }
    //     }
    //     if (flag)
    //         continue;
    //     else
    //         break;
    // }

    //*
    for (int i = 0; i < 250; i++)
    {
        for (int id_ans = 0; id_ans < X_results.size(); id_ans++)
        {
            const auto &X = X_results[id_ans];
            for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
            {
                flows_vec[site_id][id_ans] = SERVER_FLOW{id_ans, site_id, X.sum_flow_site[site_id]}; //因为是按照所有时刻计算的，所以即使为0也要计算在内
            }
        }

        if (dispath(server_supported_flow_2_site_id_vec, max_95_percent_index, flows_vec, X_results))
        {
            // std::cout << "进行了重分配" << std::endl;
        }
        else
        {
            int last_quantile_index = -1;
            bool flag = false;
            //更新一遍后5%的分位流量
            for (int i = 96; i <= 100; i++)
            {
                int quantile_index = calculate_quantile_index(double(i) / 100.0, g_site_bandwidth.site_name.size());
                if (quantile_index == last_quantile_index)
                {
                    continue;
                }
                last_quantile_index = quantile_index;
                bool flag_tmp = dispath2(server_supported_flow_2_site_id_vec, quantile_index, flows_vec, X_results);
                flag = flag || flag_tmp;
                if (flag_tmp)
                {
                    for (int id_ans = 0; id_ans < X_results.size(); id_ans++)
                    {
                        const auto &X = X_results[id_ans];
                        for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
                        {
                            flows_vec[site_id][id_ans] = SERVER_FLOW{id_ans, site_id, X.sum_flow_site[site_id]}; //因为是按照所有时刻计算的，所以即使为0也要计算在内
                        }
                    }
                }
            }
            if (flag)
                continue;
            else
                break;
        }
    }
    //*/

    {
        //做测试用，输出最终的结果的成本
        vector<int> flows_vec_95_according_site_id(g_qos.client_name.size(), 0);
        {
            int idx = calculate_quantile_index(0.95, g_qos.client_name.size());

            vector<SERVER_FLOW> flows_vec_quantile2;
            get_server_flow_vec_by_quantile(calculate_quantile_index(0.95, g_site_bandwidth.site_name.size()),
                                            flows_vec, flows_vec_quantile2, flows_vec_95_according_site_id);
        }
        int sum = std::accumulate(flows_vec_95_according_site_id.begin(), flows_vec_95_according_site_id.end(), 0);
        printf("%d\n", sum);
    }
}
