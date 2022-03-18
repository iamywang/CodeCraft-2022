#include <iostream>
#include <functional>
#include <numeric>
#include <algorithm>
#include <queue>
#include "global.hpp"

using namespace std;

typedef struct _SERVER_SUPPORTED_FLOW
{
    string mtime;
    int max_flow;
    int server_index;
} SERVER_SUPPORTED_FLOW;

typedef struct _MAX_5_PERCENT_FLOW
{
    // max priority queue
    priority_queue<int, vector<int>, greater<int>> max_flow_queue;
} MAX_5_PERCENT_FLOW;

static vector<MAX_5_PERCENT_FLOW> max_5_percent_flow_vec(g_demand.site_name.size()); //边缘节点对应的能存储的最大的5%流量

/**
 * @brief
 *
 * @param [out] server_supported_flow 边缘节点在各个时刻可提供的最大流量
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
                max_server_support_flow +=
                    std::min(g_qos.qos[server_id][client_id],
                             line_demand[client_id]);
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

/**
 * @brief
 *
 * @param [in] demand
 * @param [in] bandwidth
 * @param [in] QOS
 * @param [out] X
 */
void solve_X(const vector<int> &demand,
             const vector<int> &bandwidth,
             const vector<vector<int>> &QOS,
             const vector<SERVER_SUPPORTED_FLOW> &server_supported_flow,
             vector<vector<int>> &X //行代表客户，列表示边缘节点
)
{
    //初始化X
    X.resize(demand.size());
    for (int i = 0; i < demand.size(); i++)
    {
        X[i] = vector<int>(g_qos.site_name.size(), 0);
    }

    const int max_5_percent_flow_size = demand.size() - (int)((demand.size() - 1) * 0.95 + 1) + 1;

    for(int i = 0; i < server_supported_flow.size(); i++)
    {
        const auto &flow = server_supported_flow[i];
        if(i < max_5_percent_flow_size)
        {
            max_5_percent_flow_vec[flow.server_index].max_flow_queue.push(flow.max_flow);
        }
        else
        {
            if(flow.max_flow > max_5_percent_flow_vec[flow.server_index].max_flow_queue.top())
            {
                //TODO 想办法削峰
                max_5_percent_flow_vec[flow.server_index].max_flow_queue.pop();
                max_5_percent_flow_vec[flow.server_index].max_flow_queue.push(flow.max_flow);
            }
        }

        for(int j = 0; j < demand.size(); j++)
        {
            
        }
    }

    return;
}

void write_X(const vector<vector<vector<int>>> &X_results)
{
    return;
}

int solve()
{
    // 排序
    vector<vector<SERVER_SUPPORTED_FLOW>> server_supported_flow_2_time_vec;
    sort_by_demand_and_qos(server_supported_flow_2_time_vec);

    vector<vector<vector<int>>> X_results;
    for (int i = 0; i < g_demand.mtime.size(); i++)
    {
        X_results.push_back(vector<vector<int>>());
    }

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

        vector<vector<int>> X;
        solve_X(demand_at_mtime, g_site_bandwidth.bandwidth, g_qos.qos, server_supported_flow, X);

        X_results[g_demand.get(mtime)] = X;
    }

    // 写出
    write_X(X_results);

    return 0;
}
