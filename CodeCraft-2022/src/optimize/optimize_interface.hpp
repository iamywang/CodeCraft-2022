#pragma once

#include "../global.hpp"

namespace optimize
{
    int solve(const int num_iteration, const bool is_generate_initial_results, std::vector<ANSWER> &X_results);

    extern DEMAND g_demand;

    typedef struct _SERVER_FLOW
    {
        int ans_id;  //记录对应流量所在解的位置
        int site_id; //记录对应流量的边缘节点
        int flow;    //当前流量
    } SERVER_FLOW;

}
