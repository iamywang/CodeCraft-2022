#include "DataIn.hpp"
#include "CSV.hpp"
#include "../global.hpp"
#include <iostream>
#include "../utils/utils.hpp"
#include "../utils/tools.hpp"

using namespace global;

// const static std::string base_path("/data/data_1/");
#ifdef __linux__
const static std::string base_path("/data/");
// const static std::string split_str("\r\n");
#else
const static std::string base_path("C:\\Users\\wangzhankun\\Desktop\\huawei\\fusai\\data\\");
// const static std::string split_str("\n");
#endif

const static char *file_names[] = {
    "config.ini",
    "demand.csv",
    "qos.csv",
    "site_bandwidth.csv",
    NULL};

std::map<std::string, int> client_idx;
std::map<std::string, int> site_idx;

void read_configure(int &qos_constraint, int &minimum_cost)
{

    std::string file_content;
    MyUtils::Tools::readAll(base_path + file_names[0], file_content);
    auto lines = MyUtils::Tools::split(file_content, split_str);
    {
        auto tmp = MyUtils::Tools::split(lines[1], "=");
        qos_constraint = std::stoi(tmp[1]);

        tmp = MyUtils::Tools::split(lines[2], "=");
        minimum_cost = std::stoi(tmp[1]);
    }
}

void read_site_bandwidth(SITE_BANDWIDTH &site_bandwidth)
{

    CSV site_bandwidth_csv(base_path + file_names[3]);
    auto &site_bandwidth_vec = site_bandwidth.bandwidth;
    auto &site_name = site_bandwidth.site_name;

    int sidx = 0;
    for (auto &con : site_bandwidth_csv.m_content)
    {
        site_name.push_back(con[0]);
        site_idx.insert(std::make_pair(con[0], sidx++));
        site_bandwidth_vec.push_back(std::stoi(con[1]));
        // site_bandwidth_vec.push_back(int(std::stoi(con[1]) * 0.2));
    }

    for (int id = 0; id < g_site_bandwidth.bandwidth.size(); id++)
    {
        g_site_bandwidth.bandwidth_sorted_by_bandwidth_ascending_order.push_back({id, g_site_bandwidth.bandwidth[id]});
    }
    std::sort(g_site_bandwidth.bandwidth_sorted_by_bandwidth_ascending_order.begin(),
              g_site_bandwidth.bandwidth_sorted_by_bandwidth_ascending_order.end(),
              [](const SITE_BANDWIDTH::_inner_data &a, const SITE_BANDWIDTH::_inner_data &b)
              {
                  return a.bandwidth > b.bandwidth;
              });

    g_num_server = g_site_bandwidth.bandwidth.size();

    return;
}

void read_demand(DEMAND &dem)
{

    CSV demand_csv(base_path + file_names[1]);
    auto &demand_vec = dem.stream_client_demand;
    auto &mtime = dem.mtime;
    auto &client_name = dem.client_name;

    int cidx = 0;
    for (auto p = demand_csv.m_header.begin() + 2; p != demand_csv.m_header.end(); p++)
    {
        client_name.push_back(*p);
        client_idx.insert(std::make_pair(*p, cidx++));
    }

    {
        std::string last_time = "";
        for (auto &con : demand_csv.m_content)
        {
            // 判断是否为同一时刻
            if (con[0] != last_time)
            {
                mtime.push_back(con[0]);
                last_time = con[0];

                STREAM_CLIENT_DEMAND stream_client_demand;
                stream_client_demand.id_local_stream_2_stream_name.push_back(con[1]);
                stream_client_demand.stream_name_2_id_local_stream.insert(std::make_pair(con[1], 0));
                std::vector<int> tmp;
                for (auto p = con.begin() + 2; p != con.end(); p++)
                {
                    tmp.push_back(std::stoi(*p));
                }
                stream_client_demand.stream_2_client_demand.push_back(tmp);

                demand_vec.push_back(stream_client_demand);
            }
            else
            {
                STREAM_CLIENT_DEMAND &stream_client_demand = demand_vec.back();
                stream_client_demand.id_local_stream_2_stream_name.push_back(con[1]);
                stream_client_demand.stream_name_2_id_local_stream.insert(std::make_pair(con[1], stream_client_demand.id_local_stream_2_stream_name.size() - 1));
                std::vector<int> tmp;
                for (auto p = con.begin() + 2; p != con.end(); p++)
                {
                    tmp.push_back(std::stoi(*p));
                }
                stream_client_demand.stream_2_client_demand.push_back(tmp);
            }
        }
    }

    g_num_client = dem.stream_client_demand[0].stream_2_client_demand[0].size();

    {
        global::g_demand.client_demand.resize(dem.stream_client_demand.size());
        auto task = [&](int t)
        {
            auto &stream_client_demand = dem.stream_client_demand[t];
            auto &client_demand = dem.client_demand[t];
            for (int id_client = 0; id_client < g_num_client; id_client++)
            {
                client_demand.push_back(MyUtils::Tools::sum_column(stream_client_demand.stream_2_client_demand, id_client));
            }
        };

        auto ret_vecs = parallel_for(0, dem.mtime.size(), task);
        for (auto &ret : ret_vecs)
        {
            ret.get();
        }
    }

    {
        // for(int t = 0; t < dem.mtime.size(); t++)
        // {
        //     auto &client_demand = dem.client_demand[t];
        //     if(client_demand.size() != g_num_client)
        //     {
        //         std::cout << "client_demand.size() != g_num_client" << std::endl;
        //         exit(0);
        //     }
        // }
    }

    global::g_demand.init(); //防止后续多线程由于都是第一次访问get导致的同时写问题

    return;
}

void read_qos(QOS &qos)
{

    CSV qos_csv(base_path + file_names[2]);
    std::vector<std::vector<int>> &qos_vec = qos.qos;
    std::vector<std::string> &client_name = qos.client_name;
    std::vector<std::string> &site_name = qos.site_name;

    client_name.resize(g_demand.client_name.size());
    site_name.resize(g_site_bandwidth.site_name.size());

    qos_vec.resize(g_site_bandwidth.site_name.size());

    for (auto p = qos_csv.m_header.begin() + 1; p != qos_csv.m_header.end(); p++)
    {
        int cidx = client_idx.find(*p)->second;
        client_name[cidx] = *p;
    }

    for (auto &con : qos_csv.m_content)
    {
        int sidx = site_idx.find(con[0])->second;
        site_name[sidx] = con[0];
        qos_vec[sidx].resize(g_demand.client_name.size());

        for (auto p = con.begin() + 1; p != con.end(); p++)
        {
            int cidx = client_idx.find(*(p - con.begin() + qos_csv.m_header.begin()))->second;
            int tmp_int = std::stoi(*p);
            if (tmp_int >= g_qos_constraint)
            {
                tmp_int = 0;
            }
            qos_vec[sidx][cidx] = tmp_int;
        }
    }

    // {
    //     for(int id_client = 0; id_client < g_num_client; id_client++)
    //     {
    //         for(int id_server = 0; id_server < g_num_server; id_server++)
    //         {
    //             std::cout << qos.qos[id_server][id_client] << " ";
    //         }
    //         std::cout << "\n";
    //     }
    // }

    return;
}

void set_90_quantile_server_id()
{
    for(int i = 0;  i < 10; i++)
    {
        g_90_percent_server_id_set.insert(g_site_bandwidth.bandwidth_sorted_by_bandwidth_ascending_order[i].id_server);
    }
}
