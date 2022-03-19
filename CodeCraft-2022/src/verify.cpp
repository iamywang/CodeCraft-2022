#include "global.hpp"
#include <iostream>
#include <numeric>

using namespace std;

bool verify(const vector<ANSWER> &X_results)
{
    for (const auto &X : X_results)
    {
        const auto &flow = X.flow;

        for (int site_id = 0; site_id < g_qos.site_name.size(); site_id++)
            if (X.sum_flow_site[site_id] > g_site_bandwidth.bandwidth[site_id])
            {
                cout << X.mtime << " client " << site_id << " flow " << X.sum_flow_site[site_id] << " > " << g_site_bandwidth.bandwidth[site_id] << endl;
                return false;
            }

        for (int client_id = 0; client_id < flow.size(); client_id++)
        {
            for (int j = 0; j < flow[client_id].size(); j++)
            {
                if (flow[client_id][j] && g_qos.qos[j][client_id] <= 0)
                {
                    cout << X.mtime << " client " << g_qos.client_name[client_id] << " server " << g_qos.site_name[j] << endl;
                    return false;
                }
            }
            int sum = std::accumulate(flow[client_id].begin(), flow[client_id].end(), 0); //客户的流量总和
            if (sum != g_demand.demand[g_demand.get(X.mtime)][client_id])
            {
                cout << X.mtime << " " << client_id << " " << sum << " " << g_demand.demand[g_demand.get(X.mtime)][client_id] << endl;
                cout << "sum is not equal demand" << endl;
                return false;
            }
        }
    }
    return true;
}