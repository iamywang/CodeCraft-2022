#include "DataIn.hpp"
#include "CSV.hpp"
#include "../global.hpp"

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

int read_qos_constraint()
{
    if (generate_fake_data)
    {
        return 400;
    }
    else
    {
        int qos_constraint(0);
        std::string file_content;
        MyUtils::Tools::readAll(base_path + file_names[0], file_content);
        auto lines = MyUtils::Tools::split(file_content, "\r\n");
        {
            auto tmp = MyUtils::Tools::split(lines[1], "=");
            qos_constraint = std::stoi(tmp[1]);
        }
        return qos_constraint;
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
            demand_vec[i].resize(client_num);
            mtime[i] = "mtime_" + std::to_string(i);
            for (int j = 0; j < client_num; j++)
            {
                demand_vec[i][j] = rand() % 10000 + 1;
            }
        }
    }
    else
    {
        CSV demand(base_path + file_names[1]);
        auto &demand_vec = g_demand.demand;
        auto &mtime = g_demand.mtime;

        for (auto &con : demand.m_content)
        {
            std::vector<int> tmp;
            mtime.push_back(con[0]);
            for (auto p = con.begin() + 1; p != con.end(); p++)
            {
                tmp.push_back(std::stoi(*p));
            }
            demand_vec.push_back(tmp);
        }
    }

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

        for (auto &con : site_bandwidth_csv.m_content)
        {
            site_name.push_back(con[0]);
            site_bandwidth_vec.push_back(std::stoi(con[1]));
            // site_bandwidth_vec.push_back(int(std::stoi(con[1]) * 0.2));
        }
    }

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
                if(qos_vec[i][j] >= g_qos_constraint)
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

        for (auto p = qos_csv.m_header.begin() + 1; p != qos_csv.m_header.end(); p++)
        {
            client_name.push_back(*p);
        }

        for (auto &con : qos_csv.m_content)
        {
            std::vector<int> tmp;
            site_name.push_back(con[0]);
            for (auto p = con.begin() + 1; p != con.end(); p++)
            {
                int tmp_int = std::stoi(*p);
                if (tmp_int >= g_qos_constraint)
                {
                    tmp_int = 0;
                }
                tmp.push_back(tmp_int);
            }
            qos_vec.push_back(tmp);
        }
    }

    return;
}
