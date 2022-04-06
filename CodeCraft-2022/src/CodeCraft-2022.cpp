#include <iostream>
#include "utils/tools.hpp"
#include "global.hpp"
#include "data_in_out/DataIn.hpp"
#include "utils/ProcessTimer.hpp"
#include "optimize/optimize_interface.hpp"
#include "solve/Solver.hpp"
#include "utils/Verifier.hpp"
#include "utils/Thread/ThreadPoll.hpp"
#include "generate/generate.hpp"
#include "heuristic/HeuristicAlgorithm.hpp"
#include "DivideConquer.hpp"

#define MULTI_THREAD

extern void write_result(const std::vector<ANSWER> &X_results);

int main()
{
    g_start_time = MyUtils::Tools::getCurrentMillisecs();

    read_configure(g_qos_constraint, g_minimum_cost);
    read_site_bandwidth(g_site_bandwidth);
    read_demand(global::g_demand); //要求最后读demand
    read_qos(g_qos);

    {
        cout << "时刻数量: " << global::g_demand.mtime.size() << endl;
        cout << "客户端数量：" << global::g_demand.client_name.size() << endl;
        cout << "服务器数量: " << g_qos.site_name.size() << endl;
        cout << "最小成本: " << g_minimum_cost << endl;
    }

    std::vector<ANSWER> X_results;

//*
#ifdef MULTI_THREAD
    {
        vector<ANSWER> X_results_tmp[NUM_THREAD];
        {
            vector<std::future<bool>> rets_vec;
            const int step = (int)global::g_demand.client_demand.size() / NUM_THREAD + 1;
            for (int i = 0; i < NUM_THREAD; i++)
            {
                int left = i * step;
                int right = (i + 1) * step - 1;
                if (right >= global::g_demand.client_demand.size())
                    right = global::g_demand.client_demand.size() - 1;
                rets_vec.push_back(g_thread_pool.commit([=, &X_results_tmp]()
                                                        { return DivideConquer::divide_conquer(left, right, X_results_tmp[i]); }));
            }
            for (auto &ret : rets_vec)
            {
                ret.get();
            }
        }

        vector<vector<ANSWER> *> X_results_vec_for_last_merge;
        {
            vector<std::future<bool>> rets_vec;
            int idx_begin = 0;
            for (int i = 0; i < NUM_THREAD - 1; i += 2)
            {
                for (auto &X : X_results_tmp[i + 1])
                {
                    X_results_tmp[i].push_back(std::move(X));
                }

                X_results_vec_for_last_merge.push_back(&X_results_tmp[i]);

                rets_vec.push_back(g_thread_pool.commit([=, &X_results_tmp]()
                                                        { return DivideConquer::task(idx_begin,
                                                                                     idx_begin + X_results_tmp[i].size() - 1,
                                                                                     300,
                                                                                     false,
                                                                                     X_results_tmp[i]); }));
                idx_begin += X_results_tmp[i].size();
            }
            for (auto &ret : rets_vec)
            {
                ret.get();
            }
        }

        for (int i = 0; i < X_results_vec_for_last_merge.size(); ++i)
        {
            for (auto &X : *X_results_vec_for_last_merge[i])
            {
                X_results.push_back(std::move(X));
            }
        }
        DivideConquer::task(0, global::g_demand.client_demand.size() - 1, 1000, false, X_results);
    }
    //*/
#else
    {
        // divide_conquer(0, global::g_demand.client_demand.size() - 1, X_results);
        task(0, global::g_demand.client_demand.size() - 1, 100, true, X_results);
    }
#endif

    generate::allocate_flow_to_stream(X_results);

    heuristic::HeuristicAlgorithm heuristic_algorithm(X_results);
    heuristic_algorithm.optimize();

    write_result(*heuristic_algorithm.m_best_X_results);

    printf("Total time: %lld ms\n", MyUtils::Tools::getCurrentMillisecs() - g_start_time);

    return 0;
}
