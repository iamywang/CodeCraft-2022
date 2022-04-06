#pragma once

#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include "utils/Thread/ThreadPoll.hpp"

extern int g_qos_constraint;
extern int g_minimum_cost; //最低成本
extern int64_t g_start_time;
extern const int G_TOTAL_DURATION;
extern int g_num_client;
extern int g_num_server;

#define NUM_THREAD 4

extern MyUtils::Thread::ThreadPool g_thread_pool;

typedef struct _SITE_BANDWIDTH
{
    std::vector<std::string> site_name;
    std::vector<int> bandwidth;

    struct _inner_data
    {
        int id_server;
        int bandwidth;
    };
    std::vector<_inner_data> bandwidth_sorted_by_bandwidth_ascending_order; //根据banddwidth从大到小排序

} SITE_BANDWIDTH;
extern SITE_BANDWIDTH g_site_bandwidth;

typedef struct _STREAM_CLIENT_DEMAND
{
    std::vector<std::string> id_local_stream_2_stream_name;
    std::map<std::string, int> stream_name_2_id_local_stream;
    std::vector<std::vector<int>> stream_2_client_demand; //各行是各个流对每个客户的需求。列是客户端的id
} STREAM_CLIENT_DEMAND;

typedef struct _DEMAND
{
    static struct _DEMAND s_demand; //记录全局的demand
    std::vector<std::string> client_name;
    std::vector<std::string> mtime;
    std::vector<std::vector<int>> client_demand; //每个客户端的总需求
    std::vector<STREAM_CLIENT_DEMAND> stream_client_demand;

private:
    std::map<std::string, int> mtime_2_id;

public:
    void init()
    {
        if (mtime_2_id.empty())
        {
            for (int i = 0; i < mtime.size(); i++)
            {
                mtime_2_id[mtime[i]] = i;
            }
        }
    }

    /**
     * @brief 获取对应mtime的demand的索引。这里获取的是局部索引
     *
     * @param time
     * @return int
     */
    int get(const std::string &time) 
    {
        int ret = mtime_2_id[time];
        return ret;
    }

    int get_global_index(const std::string &time)
    {
        int ret = s_demand.get(time);
        return ret;
    }

    /**
     * @brief 获取[left, right]
     *
     * @param [out] dem
     * @param [in] left
     * @param [in] right
     */
    // static void slice(_DEMAND &dem, int left, int right)
    // {
    //     dem.clear();
    //     for (int i = left; i <= right; i++)
    //     {
    //         dem.stream_client_demand.push_back(s_demand.stream_client_demand[i]);
    //         dem.mtime.push_back(s_demand.mtime[i]);
    //         dem.client_demand.push_back(s_demand.client_demand[i]);
    //     }
    // }

    // void clear()
    // {
    //     mtime_2_id.clear();
    //     mtime.clear();
    //     stream_client_demand.clear();
    //     client_demand.clear();
    // }

} DEMAND;

namespace global
{
    extern DEMAND &g_demand;
}

typedef struct _QOS
{
    std::vector<std::string> client_name;
    std::vector<std::string> site_name;
    std::vector<std::vector<int>> qos; //举例qos[0]数组表示边缘节点与各个客户端的网络时延。
} QOS;
extern QOS g_qos;

typedef struct _ANSWER
{
    int idx_global_mtime;                           //该解在当前时刻在global::g_demand中对应的时刻的index
    std::vector<std::vector<int>> flow;             //行是客户，列是边缘节点
    std::vector<std::vector<int>> stream2server_id; //行是stream,列是client，值是server_id。由于相应的stream对client的需求可以为0，因此对应的值会是-1
    std::vector<int> sum_flow_site;                 //各个服务器的实际总流量
    std::vector<double> cost;                       //各个服务器的成本

    _ANSWER(){}
    _ANSWER(_ANSWER&& rhs)
    {
        idx_global_mtime = rhs.idx_global_mtime;
        flow = std::move(rhs.flow);
        stream2server_id = std::move(rhs.stream2server_id);
        sum_flow_site = std::move(rhs.sum_flow_site);
        cost = std::move(rhs.cost);
    }
    _ANSWER& operator=(_ANSWER&& rhs)
    {
        idx_global_mtime = rhs.idx_global_mtime;
        flow = std::move(rhs.flow);
        stream2server_id = std::move(rhs.stream2server_id);
        sum_flow_site = std::move(rhs.sum_flow_site);
        cost = std::move(rhs.cost);
        return *this;
    }
    _ANSWER(const _ANSWER& rhs)
    {
        idx_global_mtime = rhs.idx_global_mtime;
        flow = rhs.flow;
        stream2server_id = rhs.stream2server_id;
        sum_flow_site = rhs.sum_flow_site;
        cost = rhs.cost;
    }
    _ANSWER& operator=(const _ANSWER& rhs)
    {
        idx_global_mtime = rhs.idx_global_mtime;
        flow = rhs.flow;
        stream2server_id = rhs.stream2server_id;
        sum_flow_site = rhs.sum_flow_site;
        cost = rhs.cost;
        return *this;
    }
    ~_ANSWER() {}

    /**
     * @brief 只允许在初始化ANSWER时使用一次
     * 
     * @param num_stream 
     */
    void init(const int num_stream)
    {
        flow.resize(g_num_client, std::vector<int>(g_num_server, 0));
        stream2server_id.resize(num_stream, std::vector<int>(g_num_client, -1));
        sum_flow_site.resize(g_num_server, 0);
        cost.resize(g_num_server, 0);
    }

    static void swap(_ANSWER &a, _ANSWER &b)
    {
        std::swap(a.idx_global_mtime, b.idx_global_mtime);
        a.flow.swap(b.flow);
        a.stream2server_id.swap(b.stream2server_id);
        a.sum_flow_site.swap(b.sum_flow_site);
        a.cost.swap(b.cost);
    }

    /**
     * @brief 设置id_stream||id_client对应要分配的服务器编号，并计算相应服务器的总流量和成本。
     * 如果该流之前就已经被分配了某个服务器，那么之前的服务器的流量和成本也会相应调整。
     *
     * @return void
     */
    void set(const int id_stream, const int id_client, const int id_server)
    {
        // const int demand_tmp = global::g_demand.stream_client_demand[idx_global_mtime].stream_2_client_demand[id_stream][id_client];
        // if (stream2server_id[id_stream][id_client] != -1) //如果已经指定了服务器
        // {
            // int id = stream2server_id[id_stream][id_client];
            // sum_flow_site[id] -= demand_tmp;
            // cost[id] = calculate_cost(id, sum_flow_site[id]);
        // }

        stream2server_id[id_stream][id_client] = id_server;
        // sum_flow_site[id_server] += demand_tmp;
        // cost[id_server] = calculate_cost(id_server, sum_flow_site[id_server]);
    }

    /**
     * @brief 计算服务器id_server的成本
     *
     * @param id_server
     * @param flow 当前的实际流量
     * @return double 成本
     */
    static double calculate_cost(const int id_server, const int flow)
    {
        double cost = 0;
        if (0 < flow)
        {
            if (flow < g_minimum_cost)
            {
                cost = g_minimum_cost;
            }
            else
            {
                cost = double((flow - g_minimum_cost) * (flow - g_minimum_cost)) / (double(g_site_bandwidth.bandwidth[id_server])) + flow;
            }
        }
        return cost;
    }

} ANSWER;

typedef struct _SERVER_SUPPORTED_FLOW
{
    int idx_global_mtime; //该解在当前时刻在global::g_demand中对应的时刻的index
    int max_flow;
    int server_index;
} SERVER_SUPPORTED_FLOW;
