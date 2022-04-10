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

// #define MULTI_THREAD
// #define STREAM_OPTIMIZE_DIVIDE
// #define HEURISTIC_ALGORITHM

extern void write_result(const std::vector<ANSWER> &X_results);

int main()
{
    g_start_time = MyUtils::Tools::getCurrentMillisecs();

    read_configure(g_qos_constraint, g_minimum_cost);
    read_site_bandwidth(g_site_bandwidth);
    read_demand(global::g_demand); //要求最后读demand
    read_qos(g_qos);
    set_90_quantile_server_id();

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
                                                        { return DivideConquer::divide_conquer<true>(left, right, X_results_tmp[i]); }));
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
#ifndef HEURISTIC_ALGORITHM
        DivideConquer::task(0, global::g_demand.client_demand.size() - 1, 100, false, X_results);
#ifndef COMPETITION
        printf("%s %d: before stream dispath, best price is %d\n", __func__, __LINE__, Verifier::calculate_price(X_results));
#endif
#endif
    }
    //*/
#else
    {
        // DivideConquer::task(0, global::g_demand.client_demand.size() - 1, 1000, true, X_results);
    }
#endif
    // generate::allocate_flow_to_stream(X_results);

    {
        std::vector<int> idx_global_demand;
        for (int i = 0; i < global::g_demand.client_demand.size(); i++)
        {
            idx_global_demand.push_back(i);
        }
        solve::Solver solver(&X_results,
                             std::move(idx_global_demand));
        solver.solve(0, true);
    }


    // printf("best price is %d\n", Verifier::calculate_price(X_results));
#ifdef HEURISTIC_ALGORITHM
    heuristic::HeuristicAlgorithm heuristic_algorithm(X_results);
    heuristic_algorithm.optimize();

    write_result(*heuristic_algorithm.m_best_X_results);
#else
    {

#ifdef STREAM_OPTIMIZE_DIVIDE

        std::vector<vector<ANSWER>> X_results_vec(NUM_THREAD);
        if (MyUtils::Tools::getCurrentMillisecs() - g_start_time < G_TOTAL_DURATION * 1000)
        {
            {
                const int step = X_results.size() / NUM_THREAD + 1;
                int left = 0;
                for (; left < X_results.size(); left += step)
                {
                    int right = left + step;
                    if (right > X_results.size())
                        right = X_results.size();
                    auto &vec = X_results_vec[left / step];
                    for (int i = left; i < right; i++)
                    {
                        vec.push_back(std::move(X_results[i]));
                    }
                }

                auto &vec = X_results_vec[3];
                while (left < X_results.size())
                {
                    vec.push_back(std::move(X_results[left++]));
                }
            }

            {
                // MyUtils::MyTimer::ProcessTimer timer;
                auto task = [&X_results_vec](const int i)
                {
                    auto X_results = X_results_vec[i];
                    std::vector<int> idx_global_demand;
                    for (auto &X : X_results)
                    {
                        idx_global_demand.push_back(X.idx_global_mtime);
                    }
                    solve::Solver solver(&X_results, std::move(idx_global_demand));
                    solver.solve_stream(500);
                };

                auto rets = parallel_for(0, NUM_THREAD, task);
                for (auto &ret : rets)
                {
                    ret.get();
                }
                // printf("%s %d: duration is %dms\n", __func__, __LINE__, timer.duration());
                // exit(-1);
            }

            X_results.resize(0);
            for (int i = 0; i < NUM_THREAD; i++)
            {
                for (auto &X : X_results_vec[i])
                {
                    X_results.push_back(std::move(X));
                }
            }
        }
#endif
    }

    std::vector<int> idx_global_demand;
    for (auto &X : X_results)
    {
        idx_global_demand.push_back(X.idx_global_mtime);
    }
    solve::Solver solver(&X_results, std::move(idx_global_demand));
    solver.solve_stream(10000);

#ifdef TEST
    Verifier::verify(X_results);
    Verifier::verify_stream(X_results);
#endif

    write_result(X_results);
#ifndef COMPETITION
    printf("%s %d, best price is %d\n", __func__, __LINE__, Verifier::calculate_price(X_results));
#endif
#endif

    printf("Total time: %lld ms\n", MyUtils::Tools::getCurrentMillisecs() - g_start_time);

    return 0;
}
