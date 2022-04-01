#include "../global.hpp"
#include <iostream>
#include <cstring>

using namespace global;

#ifdef _WIN32
static const char *output_path = "./"
								 "solution.txt";
#else
static const char *output_path = "/output/"
								 "solution.txt";
#endif

void write_result(const std::vector<ANSWER> &X_results)
{
	FILE *fp = fopen(output_path, "w");
	if (fp == NULL)
	{
		printf("open file %s failed\n", output_path);
		exit(1);
	}

	for (int time = 0; time < X_results.size(); time++)
	{
		auto &X = X_results[time];

		for (int client_id = 0; client_id < g_qos.client_name.size(); client_id++)
		{
			char buf[409600];
			char *p_buf = buf;
			std::vector<std::string> tmp_server;
			tmp_server.resize(g_qos.site_name.size());
			sprintf(p_buf, "%s:", g_qos.client_name[client_id].c_str());
			p_buf += strlen(p_buf);

			std::vector<std::string> &stream_names = g_demand.demand[time].id_local_stream_2_stream_name;
			for (int stream_id = 0; stream_id < stream_names.size(); stream_id++)
			{
				int server_id = X.stream2server_id[stream_id][client_id];
				if (server_id >= 0)
				{
					if (tmp_server[server_id] == "")
					{
						tmp_server[server_id] = "<" + g_qos.site_name[server_id];
					}
					tmp_server[server_id] += "," + stream_names[stream_id];
				}
			}
			bool flag = false;
			for (int server_id = 0; server_id < tmp_server.size(); server_id++)
			{
				if (tmp_server[server_id] != "")
				{
					flag = true;
					sprintf(p_buf, "%s>,", tmp_server[server_id].c_str());
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
