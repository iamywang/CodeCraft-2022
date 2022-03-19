#include <iostream>
#include <functional>
#include <numeric>
#include <algorithm>
#include <queue>
#include "global.hpp"
#include "tools.hpp"

using namespace std;

typedef struct _MAX_5_PERCENT_FLOW
{
    // max priority queue
    priority_queue<int, vector<int>, greater<int>> max_flow_queue;
} MAX_5_PERCENT_FLOW;

static vector<MAX_5_PERCENT_FLOW> max_5_percent_flow_vec; //边缘节点对应的能存储的最大的5%流量

/**
 * @brief
 *
 * @param [out] server_supported_flow 边缘节点在各个时刻可提供的最大流量。每一行都是每个时刻各个边缘节点可以为当前所有客户端提供的最大流量。
 */
void sort_by_demand_and_qos(vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_time_vec)
{

    for (int k = 0; k < g_demand.demand.size(); k++)
    {
        vector<SERVER_SUPPORTED_FLOW> server_supported_flow;

        const auto &line_demand = g_demand.demand[k];

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
                g_demand.mtime[k], max_server_support_flow, server_id});
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
        if (max_5_percent_flow_vec[server_supported_flow[i].server_index].max_flow_queue.size() > max_5_percent_flow_size)
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
        max_5_percent_flow_vec[site_index].max_flow_queue.push(server_supported_flow[site_index].max_flow);

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

extern void optimize(const vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_time_vec, vector<ANSWER> &X_results);
extern bool verify(const vector<ANSWER> &X_results);

int solve(std::vector<ANSWER> &X_results)
{
    // 排序
    vector<vector<SERVER_SUPPORTED_FLOW>> server_supported_flow_2_time_vec;
    sort_by_demand_and_qos(server_supported_flow_2_time_vec);

    // vector<ANSWER> X_results;
    X_results.resize(g_demand.mtime.size());

    max_5_percent_flow_vec.resize(g_qos.site_name.size());
    // 求解每一时刻的X
    for (auto &server_supported_flow : server_supported_flow_2_time_vec)
    {
        const auto &mtime = server_supported_flow[0].mtime;
        int index = g_demand.get(mtime);
        if (index == -1)
        {
            char buf[1024];
            sprintf(buf, "mtime: %s not found in demand", mtime.c_str());
            throw runtime_error(string(buf));
        }

        //取出对应时刻的demand
        vector<int> &demand_at_mtime = g_demand.demand[g_demand.get(mtime)];

        ANSWER ans;
        ans.mtime = mtime;
        solve_X(demand_at_mtime, g_site_bandwidth.bandwidth, g_qos.qos, server_supported_flow, ans);

        X_results[g_demand.get(mtime)] = ans;
    }

    for (auto &X : X_results)
    {
        for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
        {
            int tmp = MyUtils::Tools::sum_column(X.flow, site_id);
            X.sum_flow_site.push_back(tmp);
        }
    }

    //根据时间对X_results进行排序
    std::sort(X_results.begin(), X_results.end(), [](const ANSWER &a, const ANSWER &b)
              { return g_demand.get(a.mtime) < g_demand.get(b.mtime); });
    // //根据时间对server_supported_flow_2_time_vec排序
    std::sort(server_supported_flow_2_time_vec.begin(), server_supported_flow_2_time_vec.end(), [](const vector<SERVER_SUPPORTED_FLOW> &a, const vector<SERVER_SUPPORTED_FLOW> &b)
              { return g_demand.get(a[0].mtime) < g_demand.get(b[0].mtime); });
    for(auto& v : server_supported_flow_2_time_vec)
    {
        //按照server_id排序
        std::sort(v.begin(), v.end(), [](const SERVER_SUPPORTED_FLOW &a, const SERVER_SUPPORTED_FLOW &b)
                  { return a.server_index < b.server_index; });
    }
    optimize(server_supported_flow_2_time_vec, X_results);

    if (verify(X_results))
    {
        cout << "verify success" << endl;
    }

    else
    {
        printf("verify failed\n");
        return -1;
    }

    return 0;
}
