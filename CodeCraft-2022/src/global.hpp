#pragma once

#include <vector>
#include <string>
#include <map>
#include <algorithm>

typedef struct _SITE_BANDWIDTH
{
    std::vector<std::string> site_name;
    std::vector<int> bandwidth;
} SITE_BANDWIDTH;

typedef struct _DEMAND
{
    std::vector<std::string> client_name;
    std::vector<std::string> mtime;
    std::vector<std::vector<int>> demand;

private:
    std::map<std::string, int> mtime_2_id;

public:
    /**
     * @brief 获取对应mtime的demand的索引
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
        return mtime_2_id[time];
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
    std::string mtime;                  //时刻某个时刻的分配方案
    std::vector<std::vector<int>> flow; //行是客户，列是边缘节点

    std::vector<int> sum_flow_site; //各列的流量之和，也即是边缘节点分配给各个客户端的流量的总和

} ANSWER;

typedef struct _SERVER_SUPPORTED_FLOW
{
    std::string mtime;
    int max_flow;
    int server_index;
} SERVER_SUPPORTED_FLOW;

extern int g_qos_constraint;
extern SITE_BANDWIDTH g_site_bandwidth;
extern QOS g_qos;
extern int64_t g_start_time;

namespace global
{

    extern DEMAND g_demand;

}
