#pragma once
#include <queue>
#include "../global.hpp"

namespace solve
{
    using namespace std;

    typedef struct _MAX_5_PERCENT_FLOW
    {
        // max priority queue
        std::priority_queue<int, std::vector<int>, greater<int>> max_flow_queue;
    } MAX_5_PERCENT_FLOW;

    typedef struct _CommonDataForResultGenrator
    {
        std::vector<MAX_5_PERCENT_FLOW> m_max_5_percent_flow_vec; //边缘节点对应的能存储的最大的5%流量
        std::vector<std::vector<SERVER_SUPPORTED_FLOW>> m_server_supported_flow_2_time_vec;
        // DEMAND m_demand;
        std::vector<int> m_idx_global_demand; //全局demand的ID索引
        std::vector<ANSWER>* m_X_results;     //严禁被free/delete

        _CommonDataForResultGenrator(
            // const DEMAND &demand,
            std::vector<ANSWER>* X_results,
            std::vector<int> &&idx_global_demand) : //  m_demand(demand),
                                                    m_X_results(X_results),
                                                    m_idx_global_demand(idx_global_demand)
        {
        }
        _CommonDataForResultGenrator() = delete;

    } CommonDataForResultGenrator;

} // namespace solve
