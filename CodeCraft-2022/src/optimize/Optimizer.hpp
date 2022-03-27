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
        const DEMAND& m_demand;
    public:
        Optimizer(const DEMAND& demand):m_demand(demand) {}
        ~Optimizer() {}
private:

public:
        /**
         * @brief
         *
         * @param [in] server_supported_flow_2_site_id_vec该参数是已经按照时刻排好序了的。且每一行都是按照site_id排序。
         * @param [in|out] X_results 该参数是已经按照时刻排好序了的
         */
        void optimize(const vector<vector<SERVER_SUPPORTED_FLOW>> &server_supported_flow_2_site_id_vec,
                      vector<ANSWER> &X_results,
                      const int num_iteration)
        {
            vector<vector<SERVER_FLOW>> flows_vec; //各行是对应的边缘节点在各个时刻的总的分配流量
            for (int i = 0; i < g_site_bandwidth.site_name.size(); i++)
            {
                flows_vec.push_back(vector<SERVER_FLOW>(this->m_demand.mtime.size()));
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
            const int max_95_percent_index = calculate_quantile_index(0.95,this->m_demand.mtime.size());

            Dispather dispahter(this->m_demand);
            int jiange = 50;
            if(X_results.size() > 2000)
            {
                jiange = 20;
            }
            for (int i = 0; i < num_iteration; i++)
            {
                if (i % jiange == 0 && MyUtils::Tools::getCurrentMillisecs() - g_start_time > G_TOTAL_DURATION * 1000)
                {
                    break;
                }
                for (int id_ans = 0; id_ans < X_results.size(); id_ans++)
                {
                    const auto &X = X_results[id_ans];
                    for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
                    {
                        flows_vec[site_id][id_ans] = SERVER_FLOW{id_ans, site_id, X.sum_flow_site[site_id]}; //因为是按照所有时刻计算的，所以即使为0也要计算在内
                    }
                }

                if (dispahter.dispath(server_supported_flow_2_site_id_vec, max_95_percent_index, flows_vec, X_results))
                {
                    // std::cout << "进行了重分配" << std::endl;
                }
                else
                {
                    int last_quantile_index = max_95_percent_index;
                    bool flag = false;
                    //更新一遍后5%的分位流量
                    for (int i = 96; i <= 100; i++)
                    {
                        int quantile_index = calculate_quantile_index(double(i) / 100.0,this->m_demand.mtime.size());
                        if (quantile_index == last_quantile_index)
                        {
                            continue;
                        }
                        last_quantile_index = quantile_index;
                        bool flag_tmp = dispahter.dispath2(server_supported_flow_2_site_id_vec, quantile_index, flows_vec, X_results);
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

            {
//做测试用，输出最终的结果的成本
#ifdef TEST
                vector<int> flows_vec_95_according_site_id(g_qos.client_name.size(), 0);
                {
                    int idx = calculate_quantile_index(0.95,this->m_demand.mtime.size());

                    vector<SERVER_FLOW> flows_vec_quantile2;
                    get_server_flow_vec_by_quantile(calculate_quantile_index(0.95,this->m_demand.mtime.size()),
                                                    flows_vec, flows_vec_quantile2, flows_vec_95_according_site_id);
                }
                int sum = std::accumulate(flows_vec_95_according_site_id.begin(), flows_vec_95_according_site_id.end(), 0);
                printf("%d\n", sum);
#endif
            }

        }
    };

} // namespace optimize
