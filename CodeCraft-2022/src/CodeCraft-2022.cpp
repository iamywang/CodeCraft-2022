#include <iostream>
#include "utils/tools.hpp"
#include "global.hpp"
#include "data_in_out/DataIn.hpp"
#include "utils/ProcessTimer.hpp"
#include "optimize/optimize_interface.hpp"
#include "optimize/Solver.hpp"

extern void write_result(const std::vector<ANSWER> &X_results);
extern bool verify(const std::vector<ANSWER> &X_results);

const int NUM_MINIUM = 100;    //最小分组中每组最多有多少个元素
const int NUM_ITERATION = 200; //最小分组的最大迭代次数

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
    optimize::g_demand.clear();
    for (int i = left; i <= right; i++)
    {
        optimize::g_demand.demand.push_back(global::g_demand.demand[i]);
        optimize::g_demand.mtime.push_back(global::g_demand.mtime[i]);
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
        optimize::Solver solver(optimize::g_demand);
        solver.m_X_results = X_results;
        if (solver.solve(num_iteration, is_generated_initial_results) == 0)
        {
            X_results = solver.m_X_results;
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
    if (right - left > 100)
    {
        int mid = (left + right) / 2;

        std::vector<ANSWER> X_results_right;
        if (!divide_conquer(left, mid, X_results))
            return false;
        if (!divide_conquer(mid + 1, right, X_results_right))
            return false;

        //合并
        optimize::g_demand.clear();
        for (auto &X : X_results)
        {
            optimize::g_demand.demand.push_back(global::g_demand.demand[global::g_demand.get(X.mtime)]);
            optimize::g_demand.mtime.push_back(global::g_demand.mtime[global::g_demand.get(X.mtime)]);
        }
        for (auto &X : X_results_right)
        {
            optimize::g_demand.demand.push_back(global::g_demand.demand[global::g_demand.get(X.mtime)]);
            optimize::g_demand.mtime.push_back(global::g_demand.mtime[global::g_demand.get(X.mtime)]);
            X_results.push_back(X);
        }

        if (optimize::solve(NUM_ITERATION, false, X_results) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return task(left, right, NUM_ITERATION, true, X_results);
    }
}

void test_solver(std::vector<ANSWER> &X_results)
{
    optimize::Solver solver(global::g_demand);
    optimize::g_demand = global::g_demand;
    solver.solve(1000, true);

    X_results = solver.m_X_results;
}

int main()
{
    MyUtils::MyTimer::ProcessTimer timer;

    g_qos_constraint = read_qos_constraint();
    read_demand(global::g_demand);
    read_site_bandwidth(g_site_bandwidth);
    read_qos(g_qos);

    std::vector<ANSWER> X_results;

    {
        divide_conquer(0, global::g_demand.demand.size() - 1, X_results);
    }

    // test_solver(X_results);

    if (verify(X_results))
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
