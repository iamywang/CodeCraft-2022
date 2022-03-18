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
    std::vector<std::string> mtime;
    std::vector<std::vector<int>> demand;

    /**
     * @brief 获取对应mtime的demand的索引
     *
     * @param time
     * @return int
     */
    int get(const std::string &time)
    {
        auto p = std::find(mtime.begin(), mtime.end(), time);
        if (p == mtime.end())
        {
            return -1;
        }
        else
        {
            return p - mtime.begin();
        }
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

extern int g_qos_constraint;

extern DEMAND g_demand;

extern SITE_BANDWIDTH g_site_bandwidth;

extern QOS g_qos;
