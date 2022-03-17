#include "DataIn.hpp"
#include "CSV.hpp"
#include "global.hpp"

const static std::string base_path("/mnt/c/Users/wangzhankun/Desktop/"
                                   "huawei/data/");

const static char *file_names[] = {
    "config.ini",
    "demand.csv",
    "qos.csv",
    "site_bandwidth.csv",
    NULL};

int read_qos_constraint()
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

void read_demand(DEMAND &g_demand)
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

    return;
}

void read_site_bandwidth(SITE_BANDWIDTH &site_bandwidth)
{
    CSV site_bandwidth_csv(base_path + file_names[3]);
    auto &site_bandwidth_vec = site_bandwidth.bandwidth;
    auto &site_name = site_bandwidth.site_name;

    for (auto &con : site_bandwidth_csv.m_content)
    {
        site_name.push_back(con[0]);
        site_bandwidth_vec.push_back(std::stoi(con[1]));
    }

    return;
}

void read_qos(QOS &qos)
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
            if(tmp_int >= g_qos_constraint)
            {
                tmp_int = 0;
            }
            tmp.push_back(tmp_int);
        }
        qos_vec.push_back(tmp);
    }

    return;
}
