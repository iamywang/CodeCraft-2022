#pragma once
#include "solve_datatype.hpp"

#include "../global.hpp"
#include <vector>

namespace solve
{
    class ResultGenerator
    {
        friend class Solver;

    protected:
        std::vector<MAX_5_PERCENT_FLOW> &m_max_5_percent_flow_vec; //边缘节点对应的能存储的最大的5%流量
        std::vector<std::vector<SERVER_SUPPORTED_FLOW>> &m_server_supported_flow_2_time_vec;
        // DEMAND &m_demand;
        std::vector<int> &m_idx_global_demand; //全局demand的ID索引
        std::vector<ANSWER> &m_X_results;

    public:
        ResultGenerator(CommonDataForResultGenrator &common_data) : m_max_5_percent_flow_vec(common_data.m_max_5_percent_flow_vec),
                                                                    m_server_supported_flow_2_time_vec(common_data.m_server_supported_flow_2_time_vec),
                                                                    // m_demand(common_data.m_demand),
                                                                    m_idx_global_demand(common_data.m_idx_global_demand),
                                                                    m_X_results(*common_data.m_X_results)

        {
        }

        ~ResultGenerator() {}

        /**
         * @brief 初始化解。会对ANSWER.flow, ANSWER.sum_flow_site, ANSWER.cost进行初始化。
         * 
         */
        virtual void generate_initial_X_results() = 0;
    };

} // namespace solve
