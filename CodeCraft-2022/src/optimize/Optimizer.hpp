#pragma once

#include "optimize_interface.hpp"
#include "../utils/tools.hpp"
#include <queue>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include "../utils/ProcessTimer.hpp"
#include "../utils/utils.hpp"
#include "Dispather.hpp"
#include "./optimize_utils.hpp"

using namespace std;

namespace optimize
{
    class Optimizer
    {
    private:
        // const DEMAND &m_demand;
        const std::vector<int> &m_idx_global_demand;
        vector<ANSWER> &m_X_results;

        vector<vector<SERVER_FLOW>> m_flows_vec_poll; //各行是对应的边缘节点在各个时刻的总的分配流量，保证各行是按照时间排序的
        vector<vector<SERVER_FLOW *>> flows_vec;      //各行是对应的边缘节点在各个时刻的总的分配流量，没法保证各行是按照时间排序的

    public:
        Optimizer(
            // const DEMAND &demand,
            const std::vector<int> &idx_global_demand,
            vector<ANSWER> &X_results_) : //   m_demand(demand),
                                          m_idx_global_demand(idx_global_demand),
                                          m_X_results(X_results_)
        {
            m_flows_vec_poll.resize(g_site_bandwidth.site_name.size(), vector<SERVER_FLOW>(this->m_idx_global_demand.size()));
            flows_vec.resize(g_site_bandwidth.site_name.size(), vector<SERVER_FLOW *>(this->m_idx_global_demand.size()));

            for (int id_ans = 0; id_ans < m_X_results.size(); id_ans++)
            {
                auto &X = m_X_results[id_ans];
                for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
                {
                    m_flows_vec_poll[site_id][id_ans] = SERVER_FLOW{id_ans, site_id, X.sum_flow_site[site_id], X.cost[site_id]};
                    flows_vec[site_id][id_ans] = &m_flows_vec_poll[site_id][id_ans];
                }
            }
        }
        ~Optimizer() {}

    public:
        /**
         * @brief
         *
         * @param [in] server_supported_flow_2_site_id_vec该参数是已经按照时刻排好序了的。且每一行都是按照site_id排序。
         */
        void optimize(const vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_site_id_vec,
                      const int num_iteration)
        {

            Dispather dispahter(
                // this->m_demand,
                this->m_idx_global_demand,
                m_flows_vec_poll,
                flows_vec,
                server_supported_flow_2_site_id_vec,
                m_X_results);

            int jiange = 50;
            if (m_X_results.size() > 2000)
            {
                jiange = 20;
            }
            for (int i = 0; i < num_iteration; i++)
            {
                if (i % jiange == 0 && MyUtils::Tools::getCurrentMillisecs() - g_start_time > G_TOTAL_DURATION * 1000)
                {
                    break;
                }

                if (dispahter.dispath())
                {
                    // std::cout << "进行了重分配" << std::endl;
                }
                else
                {
                    // int last_quantile_index = max_95_percent_index;
                    bool flag = false;
                    //更新一遍后5%的分位流量
                    for (int i = 96; i <= 100; i++)
                    {
                        // int quantile_index = calculate_quantile_index(double(i) / 100.0, this->m_idx_global_demand.size());
                        // if (quantile_index == last_quantile_index)
                        // {
                        //     continue;
                        // }
                        // last_quantile_index = quantile_index;
                        bool flag_tmp = dispahter.dispath2(double(i) / 100.0);
                        flag = flag || flag_tmp;
                    }
                    if (flag)
                        continue;
                    else
                        break;
                }
            }

            {
//做测试用，输出最终的结果的成本
#ifdef TEST
                vector<int> flows_vec_95_according_site_id(g_qos.client_name.size(), 0);
                {
                    int idx = calculate_quantile_index(0.95, this->m_idx_global_demand.size());

                    vector<SERVER_FLOW *> flows_vec_quantile2;
                    get_server_flow_vec_by_quantile(0.95,
                                                    flows_vec, flows_vec_quantile2, flows_vec_95_according_site_id);
                }
                int sum = std::accumulate(flows_vec_95_according_site_id.begin(), flows_vec_95_according_site_id.end(), 0);
                printf("%d\n", sum);
#endif
            }

            // auto rets = parallel_for(0, m_X_results.size(), [this](int i)
            //                          {
            //                             if(i > this->m_X_results.size())
            //                             {
            //                                 throw runtime_error("i > this->m_X_results.size()");
            //                                 exit(-1);
            //                             }
            //                             auto&X = this->m_X_results[i];
            //                             for(int id_server = 0; id_server < g_num_server; id_server++)
            //                             {
            //                                 X.cost[id_server] = ANSWER::calculate_cost(id_server, X.sum_flow_site[id_server]);
            //                             } });
            // for (auto &i : rets)
            //     i.get();
            return;
        }
    };

} // namespace optimize
