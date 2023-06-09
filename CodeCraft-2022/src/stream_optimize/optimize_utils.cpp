#include "optimize_interface.hpp"
#include "../utils/ProcessTimer.hpp"
#include "../utils/utils.hpp"
#include <iostream>
using namespace std;

namespace stream_optimize
{

    /**
     * @brief 函数会对flows_vec按照flow从小到大排序
     *
     * @param quantile 百分位数对应索引值
     * @param [in|out] cost_vec 会对 cost_vec 的值进行部分排序，只是为了拿到规定的quantile的值
     * @param [out] cost_vec_quantile_vec 只记录第quantile百分位数的的SERVER_FLOW，根据flow从小到大排序
     * @param [out] flows_vec_quantile_vec_idx 按照site_id进行索引的第quantile的流量值
     */
    void get_server_flow_vec_by_quantile(const int quantile,
                                         vector<vector<SERVER_FLOW *>> &cost_vec,
                                         vector<SERVER_FLOW *> &cost_vec_quantile,
                                         vector<int> &flows_vec_quantile_according_site_id)
    {
        const int quantile_95_idx = calculate_quantile_index(0.95, cost_vec[0].size());
        // const int quantile_idx = calculate_quantile_index(quantile, flows_vec[0].size());
        const int quantile_90_idx = calculate_quantile_index(0.9, cost_vec[0].size());

        { //提取quantile百分位数的SERVER_FLOW
            cost_vec_quantile.resize(cost_vec.size());

            auto task = [&](int idx)
            {
                int quantile_idx_tmp = quantile;
                // if (quantile == quantile_95_idx && g_90_percent_server_id_set.find(idx) != g_90_percent_server_id_set.end())
                // {
                //     quantile_idx_tmp = quantile_90_idx;
                // }
                std::nth_element(cost_vec[idx].begin(),
                                 cost_vec[idx].begin() + quantile_idx_tmp,
                                 cost_vec[idx].end(),
                                 [](const SERVER_FLOW *a, const SERVER_FLOW *b)
                                 { return *(a->cost) < *(b->cost); });
                cost_vec_quantile[idx] = (cost_vec[idx][quantile_idx_tmp]);
            };

            //测试发现，在大样本下，并行提升速度明显，小样本下速度略有下降
            /*
            for (int i = 0; i < cost_vec.size(); i++)
            {
                task(i);
            }
            //*/

            //*
            int n = g_thread_pool.idlCount();
            if (n == 0 || cost_vec.size() * cost_vec[0].size() < 10 * 800)
            // if (n == 0)
            {
                for (int i = 0; i < cost_vec.size(); i++)
                {
                    task(i);
                }
            }
            else
            {
                const int step = cost_vec.size() / (n + 1) + 1; // n+1是因为当前线程也可以处理
                vector<std::future<bool>> rets_vec;
                {
                    int left = 0;
                    for (; left < cost_vec.size() && n; left += step, --n)
                    {
                        rets_vec.push_back(g_thread_pool.commit(
                            [&, left]() -> bool
                            {
                                for (int i = left; i < left + step && i < cost_vec.size(); i++)
                                {
                                    task(i);
                                }
                                return true;
                            }));
                    }
                    for (; left < cost_vec.size(); left++)
                    {
                        task(left);
                    }
                }
                for (auto &ret : rets_vec)
                {
                    ret.get();
                }
            }
            //*/
        }

        {
            //对cost_vec_quantile从小到大排序
            std::sort(cost_vec_quantile.begin(), cost_vec_quantile.end(),
                      [](const SERVER_FLOW *a, const SERVER_FLOW *b)
                      { return *(a->cost) < *(b->cost); });
        }

        {
            flows_vec_quantile_according_site_id.resize(cost_vec_quantile.size());
            for (const auto &i : cost_vec_quantile)
            {
                flows_vec_quantile_according_site_id[i->site_id] = *i->flow;
            }
        }
    }
} // namespace optimize
