#include <iostream>
#include "utils/tools.hpp"
#include "global.hpp"
#include "data_in_out/DataIn.hpp"

using namespace std;

int main()
{
    g_start_time = MyUtils::Tools::getCurrentMillisecs();

    read_configure(g_qos_constraint, g_minimum_cost);
    read_demand(global::g_demand);
    read_site_bandwidth(g_site_bandwidth);
    read_qos(g_qos);

    for (int i = 0; i < global::g_demand.mtime.size(); i++)
    {
        printf("Time: %s, Streams: %d\n", global::g_demand.mtime[i].c_str(), global::g_demand.demand[i].id_local_stream_2_stream_name.size());
        for (int j = 0; j < global::g_demand.demand[i].id_local_stream_2_stream_name.size(); j++)
        {
            string stream_name = global::g_demand.demand[i].id_local_stream_2_stream_name[j];
            printf("Stream %d %s: ", global::g_demand.demand[i].stream_name_2_id_local_stream.find(stream_name)->second, stream_name.c_str());
            for (int k = 0; k < global::g_demand.client_name.size(); k++)
            {
                printf("%d ", global::g_demand.demand[i].stream_2_client_demand[j][k]);
            }
            printf("\n");
        }
    }

    {
        cout << "时刻数量: " << global::g_demand.mtime.size() << endl;
        cout << "客户端数量：" << global::g_demand.client_name.size() << endl;
        cout << "服务器数量: " << g_qos.site_name.size() << endl;
        cout << "最小成本: " << g_minimum_cost << endl;
    }

    return 0;
}
