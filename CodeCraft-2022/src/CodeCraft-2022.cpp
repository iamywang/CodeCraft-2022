#include <iostream>
#include "utils/tools.hpp"
#include "global.hpp"
#include "data_in_out/DataIn.hpp"
#include "utils/ProcessTimer.hpp"
#include "optimize/optimize_interface.hpp"
#include "optimize/Solver.hpp"
#include "utils/Verifier.hpp"
#include "utils/Thread/ThreadPoll.hpp"
extern void write_result(const std::vector<ANSWER> &X_results);

const int NUM_MINIUM_PER_BLOCK = 100;    //最小分组中每组最多有多少个元素
const int NUM_ITERATION = 300; //最小分组的最大迭代次数
const int NUM_THREAD = 4;
static MyUtils::Thread::ThreadPool thread_pool(NUM_THREAD);

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
    DEMAND demand;
    demand.clear();
    for (int i = left; i <= right; i++)
    {
        demand.demand.push_back(global::g_demand.demand[i]);
        demand.mtime.push_back(global::g_demand.mtime[i]);
    }

    /*
        if (optimize::solve(num_iteration, is_generated_initial_results, X_results) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
        //*/

    //*
    optimize::Solver solver(X_results, demand);
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
        // optimize::g_demand.clear();
        DEMAND demand;
        demand.clear();
        for (auto &X : X_results)
        {
            demand.demand.push_back(global::g_demand.demand[global::g_demand.get(X.mtime)]);
            demand.mtime.push_back(global::g_demand.mtime[global::g_demand.get(X.mtime)]);
        }
        for (auto &X : X_results_right)
        {
            demand.demand.push_back(global::g_demand.demand[global::g_demand.get(X.mtime)]);
            demand.mtime.push_back(global::g_demand.mtime[global::g_demand.get(X.mtime)]);
            X_results.push_back(X);
        }

        {
            // if (optimize::solve(NUM_ITERATION, false, X_results) == 0)
            // {
            //     return true;
            // }
            // else
            // {
            //     return false;
            // }
        }

        {
            int num_iteration = NUM_ITERATION;
            if(X_results.size() > 1000)
            {
                num_iteration = 200;
            }
            optimize::Solver solver(X_results, demand);
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
    MyUtils::MyTimer::ProcessTimer timer;

    g_qos_constraint = read_qos_constraint();
    read_demand(global::g_demand);
    read_site_bandwidth(g_site_bandwidth);
    read_qos(g_qos);

    std::vector<ANSWER> X_results;

    //*
    {
        vector<ANSWER> X_results_tmp[NUM_THREAD];
        vector<std::future<bool>> rets_vec;
        const int step = (int)global::g_demand.demand.size() / NUM_THREAD + 1;
        for (int i = 0; i < NUM_THREAD; i++)
        {
            int left = i * step;
            int right = (i + 1) * step - 1;
            if (right >= global::g_demand.demand.size())
                right = global::g_demand.demand.size() - 1;
            rets_vec.push_back(thread_pool.commit([=, &X_results_tmp]()
                                                  { return divide_conquer(left, right, X_results_tmp[i]); }));
        }
        bool flag = true;
        for (int i = 0; i < NUM_THREAD; i++)
        {
            flag &= rets_vec[i].get();
        }
        for (int i = 0; i < NUM_THREAD; i++)
        {
            X_results.insert(X_results.end(), X_results_tmp[i].begin(), X_results_tmp[i].end());
        }
        task(0, global::g_demand.demand.size() - 1, NUM_ITERATION, false, X_results);
    }
    //*/

    {
        // divide_conquer(0, global::g_demand.demand.size() - 1, X_results);
    }

    // test_solver(X_results);

    if (Verifier(global::g_demand).verify(X_results))
    {
        write_result(X_results);
    }
    else
    {
        printf("solve failed\n");
    }

    printf("总耗时：%d ms\n", timer.duration());

    return 0;
}
