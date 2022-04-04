#include "optimize_interface.hpp"
#include "../utils/ProcessTimer.hpp"
#include <iostream>
using namespace std;

namespace optimize
{

    /**
     * @brief 函数会对flows_vec按照flow从小到大排序
     *
     * @param quantile 百分位数对应索引值
     * @param [in|out] flows_vec 会对flows_vec的值进行部分排序，只是为了拿到规定的quantile的值
     * @param [out] flows_vec_quantile_vec 只记录第quantile百分位数的的SERVER_FLOW，根据flow从小到大排序
     * @param [out] flows_vec_quantile_vec_idx 按照site_id进行索引的第quantile的流量值
     */
    void get_server_flow_vec_by_quantile(const int quantile,
                                         vector<vector<SERVER_FLOW *>> &flows_vec,
                                         vector<SERVER_FLOW *> &flows_vec_quantile,
                                         vector<int> &flows_vec_quantile_according_site_id)
    {
        { //提取quantile百分位数的SERVER_FLOW
            flows_vec_quantile.resize(flows_vec.size());

            auto task = [&flows_vec, &flows_vec_quantile, &quantile](int idx)
            {
                //*
                std::nth_element(flows_vec[idx].begin(),
                                 flows_vec[idx].begin() + quantile,
                                 flows_vec[idx].end(),
                                 [](const SERVER_FLOW *a, const SERVER_FLOW *b)
                                 { return *a->flow < *b->flow; });
                //*/

                /*
                std::sort(flows_vec[idx].begin(), flows_vec[idx].end(), [](const SERVER_FLOW *a, const SERVER_FLOW *b)
                {
                    return a->flow < b->flow;
                });
                //*/
                flows_vec_quantile[idx] = (flows_vec[idx][quantile]);
            };

            //测试发现，在大样本下，并行提升速度明显，小样本下速度略有下降
            /*
            for (int i = 0; i < flows_vec.size(); i++)
            {
                task(i);
            }
            //*/

            //*
            int n = g_thread_pool.idlCount();
            if (n == 0 || flows_vec.size() * flows_vec[0].size() < 10 * 800)
            // if (n == 0)
            {
                for (int i = 0; i < flows_vec.size(); i++)
                {
                    task(i);
                }
            }
            else
            {
                const int step = flows_vec.size() / (n + 1) + 1; // n+1是因为当前线程也可以处理
                vector<std::future<bool>> rets_vec;
                {
                    int left = 0;
                    for (; left < flows_vec.size() && n; left += step, --n)
                    {
                        rets_vec.push_back(g_thread_pool.commit(
                            [&, left]() -> bool
                            {
                                for (int i = left; i < left + step && i < flows_vec.size(); i++)
                                {
                                    task(i);
                                }
                                return true;
                            }));
                    }
                    for (; left < flows_vec.size(); left++)
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
            //对flows_vec_quantile从小到大排序
            std::sort(flows_vec_quantile.begin(), flows_vec_quantile.end(),
                      [](const SERVER_FLOW *a, const SERVER_FLOW *b)
                      { return *a->flow < *b->flow; });
        }

        {
            flows_vec_quantile_according_site_id.resize(flows_vec_quantile.size());
            for (const auto &i : flows_vec_quantile)
            {
                flows_vec_quantile_according_site_id[i->site_id] = *i->flow;
            }
        }
    }
} // namespace optimize
