#pragma once

#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include "utils/Thread/ThreadPoll.hpp"

typedef struct _SITE_BANDWIDTH
{
    std::vector<std::string> site_name;
    std::vector<int> bandwidth;
} SITE_BANDWIDTH;

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
    // std::vector<std::vector<int>> demand;
    std::vector<STREAM_CLIENT_DEMAND> demand;

private:
    std::map<std::string, int> mtime_2_id;

public:
    /**
     * @brief 获取对应mtime的demand的索引。这里获取的是局部索引
     *
     * @param time
     * @return int
     */
    int get(const std::string &time)
    {
        if (mtime_2_id.empty())
        {
            for (int i = 0; i < mtime.size(); i++)
            {
                mtime_2_id[mtime[i]] = i;
            }
        }
        int ret = mtime_2_id[time];
        return ret;
    }

    int get_global_index(const std::string &time)
    {
        int ret = s_demand.get(time);
        return ret;
    }

    void clear()
    {
        mtime_2_id.clear();
        mtime.clear();
        demand.clear();
    }

} DEMAND;

typedef struct _QOS
{
    std::vector<std::string> client_name;
    std::vector<std::string> site_name;
    std::vector<std::vector<int>> qos; //举例qos[0]数组表示边缘节点与各个客户端的网络时延。
} QOS;

typedef struct _ANSWER
{
    int idx_global_mtime;               //该解在当前时刻在global::g_demand中对应的时刻的index
    int idx_local_mtime;                //该解在当前时刻在局部demand中对应的时刻的index
    std::string mtime;                  //时刻某个时刻的分配方案
    std::vector<std::vector<int>> stream2server_id; //行是stream,列是client，值是server_id
    std::vector<int> sum_flow_site; //各个服务器的实际总流量
    std::vector<int> cost;//各个服务器的成本

} ANSWER;

typedef struct _SERVER_SUPPORTED_FLOW
{
    int idx_global_mtime; //该解在当前时刻在global::g_demand中对应的时刻的index
    int idx_local_mtime;  //该解在当前时刻在局部demand中对应的时刻的index
    std::string mtime;
    int max_flow;
    int server_index;
} SERVER_SUPPORTED_FLOW;

extern int g_qos_constraint;
extern int g_minimum_cost;//最低成本
extern SITE_BANDWIDTH g_site_bandwidth;
extern QOS g_qos;
extern int64_t g_start_time;
extern const int G_TOTAL_DURATION;

#define NUM_THREAD 4

extern MyUtils::Thread::ThreadPool g_thread_pool;
namespace global
{

    extern DEMAND &g_demand;
}
