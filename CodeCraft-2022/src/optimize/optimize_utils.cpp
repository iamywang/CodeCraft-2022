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
     * @param [in|out] flows_vec 会对flows_vec按照flow从小到大排序
     * @param [out] flows_vec_quantile_vec 只记录第quantile百分位数的的SERVER_FLOW，根据flow从小到大排序
     * @param [out] flows_vec_quantile_vec_idx 按照site_id进行索引的第quantile的流量值
     */
    void get_server_flow_vec_by_quantile2(const int quantile,
                                          vector<vector<SERVER_FLOW>> &flows_vec,
                                          vector<SERVER_FLOW> &flows_vec_quantile,
                                          vector<int> &flows_vec_quantile_according_site_id)
    {
        for (auto &v : flows_vec)
        {
            //根据流量从小到大排序
            std::sort(v.begin(), v.end(), [](const SERVER_FLOW &a, const SERVER_FLOW &b)
                      { return a.flow < b.flow; });
        }

        { //提取quantile百分位数的SERVER_FLOW
            for (int i = 0; i < flows_vec.size(); i++)
            {
                flows_vec_quantile.push_back(flows_vec[i][quantile]);
            }
            //对flows_vec_quantile从小到大排序
            std::sort(flows_vec_quantile.begin(), flows_vec_quantile.end(),
                      [](const SERVER_FLOW &a, const SERVER_FLOW &b)
                      { return a.flow < b.flow; });
        }

        {
            flows_vec_quantile_according_site_id.resize(flows_vec_quantile.size());
            for (const auto &i : flows_vec_quantile)
            {
                flows_vec_quantile_according_site_id[i.site_id] = i.flow;
            }
        }
    }

    /**
     * @brief 函数会对flows_vec按照flow从小到大排序
     *
     * @param quantile 百分位数对应索引值
     * @param [in|out] flows_vec 会对flows_vec的值进行部分排序，只是为了拿到规定的quantile的值
     * @param [out] flows_vec_quantile_vec 只记录第quantile百分位数的的SERVER_FLOW，根据flow从小到大排序
     * @param [out] flows_vec_quantile_vec_idx 按照site_id进行索引的第quantile的流量值
     */
    void get_server_flow_vec_by_quantile(const int quantile,
                                         vector<vector<SERVER_FLOW>> &flows_vec,
                                         vector<SERVER_FLOW> &flows_vec_quantile,
                                         vector<int> &flows_vec_quantile_according_site_id)
    {
        /*
        for (auto &v : flows_vec)
        {
            //根据流量从小到大排序
            std::sort(v.begin(), v.end(), [](const SERVER_FLOW &a, const SERVER_FLOW &b)
                      { return a.flow < b.flow; });
        }
        //*/

        { //提取quantile百分位数的SERVER_FLOW
            flows_vec_quantile.resize(flows_vec.size());

            /*
            for (int i = 0; i < flows_vec.size(); i++)
            {
                std::nth_element(flows_vec[i].begin(),
                                 flows_vec[i].begin() + quantile,
                                 flows_vec[i].end(),
                                 [](const SERVER_FLOW &a, const SERVER_FLOW &b)
                                 { return a.flow < b.flow; });

                flows_vec_quantile[i] = (flows_vec[i][quantile]);
            }
            //*/

            //*
            const int n = g_thread_pool.idlCount();
            if (n < 2)
            {
                for (int i = 0; i < flows_vec.size(); i++)
                {
                    std::nth_element(flows_vec[i].begin(),
                                     flows_vec[i].begin() + quantile,
                                     flows_vec[i].end(),
                                     [](const SERVER_FLOW &a, const SERVER_FLOW &b)
                                     { return a.flow < b.flow; });

                    flows_vec_quantile[i] = (flows_vec[i][quantile]);
                }
            }
            else
            {
                const int step = flows_vec.size() / n + 1;
                vector<std::future<bool>> rets_vec;
                for (int left = 0; left < flows_vec.size(); left += step)
                {
                    rets_vec.push_back(g_thread_pool.commit(
                        [&, left]() -> bool
                        {
                            for (int i = left; i < left + step && i < flows_vec.size(); i++)
                            {
                                std::nth_element(flows_vec[i].begin(),
                                                 flows_vec[i].begin() + quantile,
                                                 flows_vec[i].end(),
                                                 [](const SERVER_FLOW &a, const SERVER_FLOW &b)
                                                 { return a.flow < b.flow; });

                                flows_vec_quantile[i] = (flows_vec[i][quantile]);
                            }
                            return true;
                        }));
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
                      [](const SERVER_FLOW &a, const SERVER_FLOW &b)
                      { return a.flow < b.flow; });
        }

        {
            flows_vec_quantile_according_site_id.resize(flows_vec_quantile.size());
            for (const auto &i : flows_vec_quantile)
            {
                flows_vec_quantile_according_site_id[i.site_id] = i.flow;
            }
        }
    }
} // namespace optimize
