#pragma once

#include "../global.hpp"

namespace optimize2
{
    typedef struct _SERVER_FLOW
    {
        int ans_id;  //记录对应流量所在解的位置
        int site_id; //记录对应流量的边缘节点
        int* flow;    //当前流量，指向ANSWER.sum_flow_site[site_id]
        double* cost; //当前流量的成本,指向ANSWER.cost[site_id]
    } SERVER_FLOW;

}
