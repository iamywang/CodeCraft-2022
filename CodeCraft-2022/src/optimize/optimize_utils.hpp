#include "optimize_interface.hpp"
using namespace std;

namespace optimize
{
    /**
     * @brief 函数会对flows_vec按照flow从小到大排序
     *
     * @param quantile 百分位数
     * @param [in|out] flows_vec 会对flows_vec按照flow从小到大排序
     * @param [out] flows_vec_quantile_vec 只记录第quantile百分位数的的SERVER_FLOW，根据flow从小到大排序
     * @param [out] flows_vec_quantile_vec_idx 按照site_id进行索引的第quantile的流量值
     */
    void get_server_flow_vec_by_quantile(const double quantile,
                                         vector<vector<SERVER_FLOW*>> &flows_vec,
                                         vector<SERVER_FLOW*> &flows_vec_quantile,
                                         vector<int> &flows_vec_quantile_according_site_id);
} // namespace optimize
