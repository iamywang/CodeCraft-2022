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

    std::vector<int> idx_global_demand;
    for (int i = left; i <= right; i++)
    {
        idx_global_demand.push_back(i);
    }
    solve::Solver solver(X_results,
                         std::move(idx_global_demand));
    if (solver.solve(num_iteration, is_generated_initial_results) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
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
        std::vector<int> idx_global_demand;
        for (auto &X : X_results)
        {
            idx_global_demand.push_back(X.idx_global_mtime);
        }
        for (auto &X : X_results_right)
        {
            idx_global_demand.push_back(X.idx_global_mtime);
            X_results.push_back(std::move(X));
        }

        {
            int num_iteration = NUM_ITERATION;
            if (X_results.size() > 1000)
            {
                num_iteration = 200;
            }
            solve::Solver solver(X_results,
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
                                                        { return divide_conquer(left, right, X_results_tmp[i]); }));
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
                                                        { return task(idx_begin,
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
        // task(0, global::g_demand.client_demand.size() - 1, 1000, false, X_results);
    }
    //*/
#else
    {
        // divide_conquer(0, global::g_demand.client_demand.size() - 1, X_results);
        task(0, global::g_demand.client_demand.size() - 1, 100, true, X_results);
    }
#endif

    std::vector<ANSWER> best_X_results;

    int last_price = INT32_MAX;

    for (int i = 0; i < 100; i++)
    {
        task(0, global::g_demand.client_demand.size() - 1, 500, false, X_results);

        {
#ifdef TEST
            if (Verifier::verify(X_results))
#endif

            {
                int price = Verifier::calculate_price(X_results);
                printf("%s: before stream dispath verify:total price is %d\n", __func__, price);
            }
#ifdef TEST
            else
            {
                printf("%s: solve failed\n", __func__);
                exit(-1);
            }
#endif
        }

        // test_solver(X_results);
        generate::allocate_flow_to_stream(X_results);

        {
#ifdef TEST
            if (Verifier::verify(X_results) && Verifier::verify_stream(X_results))
#endif
            {
                int price = Verifier::calculate_price(X_results);
                if (last_price > price)
                {
                    last_price = price;
                    best_X_results = X_results;
                    printf("best price is %d\n", last_price);
                }
                printf("%s: after stream dispath verify:total price is %d\n", __func__, price);
            }
#ifdef TEST
            else
            {
                printf("%s: solve failed\n", __func__);
                exit(-1);
            }
#endif
        }

        if (MyUtils::Tools::getCurrentMillisecs() - g_start_time > G_TOTAL_DURATION * 1000)
        {
            break;
        }
    }

    write_result(best_X_results);
    printf("best price is %d\n", last_price);

    printf("Total time: %lld ms\n", MyUtils::Tools::getCurrentMillisecs() - g_start_time);

    return 0;
}
