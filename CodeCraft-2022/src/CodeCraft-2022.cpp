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

#define MULTI_THREAD

extern void write_result(const std::vector<ANSWER> &X_results);

const int NUM_MINIUM_PER_BLOCK = 200; //最小分组中每组最多有多少个元素
const int NUM_ITERATION = 200;        //最小分组的最大迭代次数

/**
 * @brief 前闭后闭区间
 *
 * @param left
 * @param right
 * @param num_iteration
 * @param X_results
 * @return true
 * @return false
 */
bool task(const int left,
          const int right,
          const int num_iteration,
          const bool is_generated_initial_results,
          std::vector<ANSWER> &X_results)
{
    // DEMAND demand;
    // DEMAND::slice(demand, left, right);

    std::vector<int> idx_global_demand;
    for (int i = left; i <= right; i++)
    {
        idx_global_demand.push_back(i);
    }
    solve::Solver solver(X_results,
                         // demand,
                         std::move(idx_global_demand));
    if (solver.solve(num_iteration, is_generated_initial_results) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
    //*/
}

/**
 * @brief 前闭后闭区间。通过分治方法计算最优解
 *
 * @param [in] left
 * @param [in] right
 * @param [out] X_results
 * @return true
 * @return false
 */
bool divide_conquer(const int left, const int right, std::vector<ANSWER> &X_results)
{
    if (right - left > NUM_MINIUM_PER_BLOCK)
    {
        int mid = (left + right) / 2;
        std::vector<ANSWER> X_results_right;

        if (!divide_conquer(left, mid, X_results))
            return false;

        if (!divide_conquer(mid + 1, right, X_results_right))
            return false;

        //合并
        // DEMAND demand;
        std::vector<int> idx_global_demand;
        // demand.clear();
        for (auto &X : X_results)
        {
            // demand.client_demand.push_back(global::g_demand.client_demand[X.idx_global_mtime]);
            // demand.stream_client_demand.push_back(global::g_demand.stream_client_demand[X.idx_global_mtime]);
            // demand.mtime.push_back(global::g_demand.mtime[X.idx_global_mtime]);
            idx_global_demand.push_back(X.idx_global_mtime);
        }
        for (auto &X : X_results_right)
        {
            // demand.client_demand.push_back(global::g_demand.client_demand[X.idx_global_mtime]);
            // demand.stream_client_demand.push_back(global::g_demand.stream_client_demand[X.idx_global_mtime]);
            // demand.mtime.push_back(global::g_demand.mtime[X.idx_global_mtime]);
            idx_global_demand.push_back(X.idx_global_mtime);
            X_results.push_back(X);
        }

        {
            int num_iteration = NUM_ITERATION;
            if (X_results.size() > 1000)
            {
                num_iteration = 200;
            }
            solve::Solver solver(X_results,
                                 //  demand,
                                 std::move(idx_global_demand));
            if (solver.solve(num_iteration, false) == 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        return task(left, right, NUM_ITERATION, true, X_results);
    }
}

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
        vector<std::future<bool>> rets_vec;
        const int step = (int)global::g_demand.client_demand.size() / NUM_THREAD + 1;
        for (int i = 0; i < NUM_THREAD; i++)
        {
            int left = i * step;
            int right = (i + 1) * step - 1;
            if (right >= global::g_demand.client_demand.size())
                right = global::g_demand.client_demand.size() - 1;
            rets_vec.push_back(g_thread_pool.commit([=, &X_results_tmp]()
                                                    { return divide_conquer(left, right, X_results_tmp[i]); }));
        }
        bool flag = true;
        for (auto &ret : rets_vec)
        {
            flag &= ret.get();
        }
        rets_vec.clear();

        vector<vector<ANSWER> *> X_results_vec_for_last_merge;
        {
            //*
            int idx_begin = 0;
            for (int i = 0; i < NUM_THREAD - 1; i += 2)
            {
                X_results_tmp[i].insert(X_results_tmp[i].end(), X_results_tmp[i + 1].begin(), X_results_tmp[i + 1].end());
                X_results_vec_for_last_merge.push_back(&X_results_tmp[i]);

                rets_vec.push_back(g_thread_pool.commit([=, &X_results_tmp]()
                                                        { return task(idx_begin, idx_begin + X_results_tmp[i].size() - 1, 100, false, X_results_tmp[i]); }));
                idx_begin += X_results_tmp[i].size();
            }

            for (auto &ret : rets_vec)
            {
                flag &= ret.get();
            }
            rets_vec.clear();
            //*/

            /*
            for(auto& v : X_results_tmp)
            {
                X_results_vec_for_last_merge.push_back(&v);
            }
            //*/
        }

        for (int i = 0; i < X_results_vec_for_last_merge.size(); ++i)
        {
            X_results.insert(X_results.end(), X_results_vec_for_last_merge[i]->begin(), X_results_vec_for_last_merge[i]->end());
        }
        task(0, global::g_demand.client_demand.size() - 1, 1000000, false, X_results);
    }
    //*/
#else
    {
        divide_conquer(0, global::g_demand.client_demand.size() - 1, X_results);
        // task(0, global::g_demand.client_demand.size() - 1, 10000, true, X_results);
    }
#endif
    // test_solver(X_results);
    generate::allocate_flow_to_stream(X_results);

    {
        Verifier ver(global::g_demand.client_demand.size());
        if (ver.verify(X_results))
        {
            int price = ver.calculate_price(X_results);
            printf("verify:total price is %d\n", price);
        }
        else
        {
            printf("solve failed\n");
            exit(-1);
        }
    }

    write_result(X_results);

    printf("Total time: %lld ms\n", MyUtils::Tools::getCurrentMillisecs() - g_start_time);

    return 0;
}
