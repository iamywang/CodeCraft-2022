#include "optimize_interface.hpp"
using namespace std;

namespace optimize2
{
    /**
     * @brief 函数会对cost_vec按照flow从小到大排序
     *
     * @param quantile 百分位数对应索引值
     * @param [in|out] cost_vec 会对cost_vec按照flow从小到大排序
     * @param [out] cost_vec_quantile_vec 只记录第quantile百分位数的的SERVER_FLOW，根据flow从小到大排序
     * @param [out] cost_vec_quantile_vec_idx 按照site_id进行索引的第quantile的流量值
     */
    void get_server_flow_vec_by_quantile(const int quantile,
                                         vector<vector<SERVER_FLOW*>> &cost_vec,
                                         vector<SERVER_FLOW*> &cost_vec_quantile,
                                         vector<int> &cost_vec_quantile_according_site_id);
} // namespace optimize
