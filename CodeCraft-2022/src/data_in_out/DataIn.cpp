#include "DataIn.hpp"
#include "CSV.hpp"
#include "../global.hpp"
#include <iostream>

using namespace global;

// const static std::string base_path("/data/data_1/");
const static std::string base_path("/data/");

const static char *file_names[] = {
    "config.ini",
    "demand.csv",
    "qos.csv",
    "site_bandwidth.csv",
    NULL};

const static bool generate_fake_data = false;
const static int site_num = 135;
const static int client_num = 35;
const static int T = 8928;

std::map<std::string, int> client_idx;
std::map<std::string, int> site_idx;

void read_configure(int &qos_constraint, int &minimum_cost)
{
    if (generate_fake_data)
    {
        qos_constraint = 400;
        minimum_cost = 400;
    }
    else
    {
        std::string file_content;
        MyUtils::Tools::readAll(base_path + file_names[0], file_content);
        auto lines = MyUtils::Tools::split(file_content, "\r\n");
        {
            auto tmp = MyUtils::Tools::split(lines[1], "=");
            qos_constraint = std::stoi(tmp[1]);

            tmp = MyUtils::Tools::split(lines[2], "=");
            minimum_cost = std::stoi(tmp[1]);
        }
    }
}

void read_demand(DEMAND &g_demand)
{
    if (generate_fake_data)
    {
        srand((unsigned)time(NULL));
        auto &demand_vec = g_demand.demand;
        auto &mtime = g_demand.mtime;
        demand_vec.resize(T);
        mtime.resize(T);
        for (int i = 0; i < T; i++)
        {
            // demand_vec[i].resize(client_num);
            mtime[i] = "mtime_" + std::to_string(i);
            for (int j = 0; j < client_num; j++)
            {
                // demand_vec[i][j] = rand() % 10000 + 1;
            }
        }
    }
    else
    {
        CSV demand(base_path + file_names[1]);
        auto &demand_vec = g_demand.demand;
        auto &mtime = g_demand.mtime;
        auto &client_name = g_demand.client_name;

        int cidx = 0;
        for (auto p = demand.m_header.begin() + 2; p != demand.m_header.end(); p++)
        {
            client_name.push_back(*p);
            client_idx.insert(std::make_pair(*p, cidx++));
        }

        std::string last_time = "";
        for (auto &con : demand.m_content)
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

    global::g_demand.get(global::g_demand.mtime[0]); //防止后续多线程由于都是第一次访问get导致的同时写问题

    return;
}

void read_site_bandwidth(SITE_BANDWIDTH &site_bandwidth)
{
    if (generate_fake_data)
    {
        srand((unsigned)time(NULL));
        auto &bandwidth = site_bandwidth.bandwidth;
        bandwidth.resize(site_num);
        for (int i = 0; i < site_num; i++)
        {
            bandwidth[i] = 1e6;
        }
        auto &site_name = site_bandwidth.site_name;
        site_name.resize(site_num);
        for (int i = 0; i < site_num; i++)
        {
            site_name[i] = "site_" + std::to_string(i);
        }
    }
    else
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
    }

    for(int id = 0; id < g_site_bandwidth.bandwidth.size(); id++)
    {
        g_site_bandwidth.bandwidth_sorted_by_bandwidth_ascending_order.push_back({id, g_site_bandwidth.bandwidth[id]});
    }
    std::sort(g_site_bandwidth.bandwidth_sorted_by_bandwidth_ascending_order.begin(),
                g_site_bandwidth.bandwidth_sorted_by_bandwidth_ascending_order.end(),
                [](const SITE_BANDWIDTH::_inner_data &a, const SITE_BANDWIDTH::_inner_data &b) {
                    return a.bandwidth > b.bandwidth;
                });

    return;
}

void read_qos(QOS &qos)
{
    if (generate_fake_data)
    {
        srand((unsigned)time(NULL));
        auto &qos_vec = qos.qos;
        qos_vec.resize(site_num);
        for (int i = 0; i < site_num; i++)
        {
            qos_vec[i].resize(client_num);
            for (int j = 0; j < client_num; j++)
            {
                qos_vec[i][j] = rand() % 600 + 1;
                if (qos_vec[i][j] >= g_qos_constraint)
                {
                    qos_vec[i][j] = 0;
                }
            }
        }
        auto &client_name = qos.client_name;
        client_name.resize(client_num);
        for (int i = 0; i < client_num; i++)
        {
            client_name[i] = "client_" + std::to_string(i);
        }
        auto &site_name = qos.site_name;
        site_name.resize(site_num);
        for (int i = 0; i < site_num; i++)
        {
            site_name[i] = "site_" + std::to_string(i);
        }
    }
    else
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
    }

    g_num_server = g_qos.site_name.size();
    g_num_client = g_qos.client_name.size();

    return;
}
