#include "global.hpp"
#include <iostream>
#include <cstring>

// static const char *output_path = "/mnt/c/Users/wangzhankun/Desktop/huawei/SDK_C++/CodeCraft-2022/output/"
static const char *output_path = "/output/"
                                 "solution.txt";


void write_result(const std::vector<ANSWER> &X_results)
{
    FILE *fp = fopen(output_path, "w");
    if (fp == NULL)
    {
        printf("open file %s failed\n", output_path);
        exit(1);
    }

    for (const auto &X : X_results)
    {
        char buf[40960];
        for (int client_id = 0; client_id < g_qos.client_name.size(); client_id++)
        {
            char *p_buf = buf;
            bool flag = false;
            // fprintf(fp, "%s:", g_qos.client_name[client_id].c_str());
            sprintf(p_buf, "%s:", g_qos.client_name[client_id].c_str());
            p_buf += strlen(p_buf);

            for (int server_id = 0; server_id < g_qos.site_name.size(); server_id++)
            {
                if (X.flow[client_id][server_id] > 0)
                {
                    flag = true;
                    // fprintf(fp, "<%s,%d>,", g_qos.site_name[server_id].c_str(), X.flow[client_id][server_id]);
                    sprintf(p_buf, "<%s,%d>,", g_qos.site_name[server_id].c_str(), X.flow[client_id][server_id]);
                    p_buf += strlen(p_buf);
                }
            }
            // delete the last char ','
            if (flag)
            {
                // fseek(fp, -1, SEEK_CUR);
                // *(p_buf - 1) = '\0';
                p_buf--;
            }
            // fprintf(fp, "\r\n");
            sprintf(p_buf, "\r\n");
            fprintf(fp, "%s", buf);
        }
    }

    fclose(fp);

    return;
}
