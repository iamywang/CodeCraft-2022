#include "optimize_interface.hpp"
#include "../utils/ProcessTimer.hpp"
#include "../utils/utils.hpp"
#include <iostream>
using namespace std;

namespace optimize
{

    template <typename _Compare>
    inline SERVER_FLOW *
    rough_nth_element(const double quantile,
                      vector<SERVER_FLOW *> &flows_vec,
                      _Compare __comp)
    {
        if(abs(quantile - 1.0) < 1e-6)
        {
            return *std::max_element(flows_vec.begin(), flows_vec.end(), __comp);
        }
        
        const int __n = 400; //每组多少个
        std::vector<SERVER_FLOW *> __indices;

        {
            const int right = flows_vec.size();
            const auto __first = flows_vec.begin();
            const int __kth = __n * quantile;
            int left = 0;

            int k = flows_vec.size() / __n; //有多少组，实际上可能是k+1组
            for (; left < right && k; left += __n, --k)
            {
                nth_element(__first + left, __first + left + __kth, __first + left + __n, __comp);
                __indices.push_back(*(__first + left + __kth));
            }
            if (left < right)
            {
                const int __kth = (flows_vec.size() - left) * quantile;
                nth_element(__first + left, __first + left + __kth, flows_vec.end(), __comp);
                __indices.push_back(*(__first + left + __kth));
            }
        }

        int __kth = __indices.size() * quantile;
        // if(__kth >= __indices.size())
        //     __kth = __indices.size() - 1;
        nth_element(__indices.begin(), __indices.begin() + __kth, __indices.end(), __comp);

        auto ret = *(__indices.begin() + __kth);

        if(ret == NULL)
        {
            cout << "ret is NULL" << endl;
            exit(-1);
        }

        return ret;
    }

    /**
     * @brief 函数会对flows_vec按照flow从小到大排序
     *
     * @param quantile 百分位数
     * @param [in|out] flows_vec 会对flows_vec的值进行部分排序，只是为了拿到规定的quantile的值
     * @param [out] flows_vec_quantile_vec 只记录第quantile百分位数的的SERVER_FLOW，根据flow从小到大排序
     * @param [out] flows_vec_quantile_vec_idx 按照site_id进行索引的第quantile的流量值
     */
    void get_server_flow_vec_by_quantile(const double quantile,
                                         vector<vector<SERVER_FLOW *>> &flows_vec,
                                         vector<SERVER_FLOW *> &flows_vec_quantile,
                                         vector<int> &flows_vec_quantile_according_site_id)
    {
        int quantile_idx = calculate_quantile_index(quantile, flows_vec[0].size());
        auto compare_func = [](SERVER_FLOW *a, SERVER_FLOW *b)
        {
            return a->flow < b->flow;
            // return a->cost < b->cost;
        };
        { //提取quantile百分位数的SERVER_FLOW
            flows_vec_quantile.resize(flows_vec.size());

            auto task = [&](int idx)
            {
                std::nth_element(flows_vec[idx].begin(),
                                 flows_vec[idx].begin() + quantile_idx,
                                 flows_vec[idx].end(),
                                 compare_func);
                flows_vec_quantile[idx] = (flows_vec[idx][quantile_idx]);
                // flows_vec_quantile[idx] = rough_nth_element(quantile, flows_vec[idx], compare_func);
            };

            auto rets = parallel_for(0, flows_vec.size(), task);
            for (auto &ret : rets)
            {
                ret.get();
            }
        }

        {
            //对flows_vec_quantile从小到大排序
            std::sort(flows_vec_quantile.begin(), flows_vec_quantile.end(),
                      compare_func);
        }

        {
            flows_vec_quantile_according_site_id.resize(flows_vec_quantile.size());
            for (const auto &i : flows_vec_quantile)
            {
                flows_vec_quantile_according_site_id[i->site_id] = i->flow;
            }
        }
    }
} // namespace optimize
