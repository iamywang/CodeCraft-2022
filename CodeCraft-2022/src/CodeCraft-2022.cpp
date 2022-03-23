#include <iostream>
#include "utils/tools.hpp"
#include "global.hpp"
#include "data_in_out/DataIn.hpp"
#include "utils/ProcessTimer.hpp"
#include "optimize/optimize_interface.hpp"

extern void write_result(const std::vector<ANSWER> &X_results);
extern bool verify(const std::vector<ANSWER> &X_results);

const int NUM_MINIUM = 100;    //最小分组中每组最多有多少个元素
const int NUM_ITERATION = 200; //最小分组的最大迭代次数

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
        std::vector<ANSWER> X_results_left;
        std::vector<ANSWER> X_results_right;
        if (!divide_conquer(left, mid, X_results_left))
            return false;
        if (!divide_conquer(mid + 1, right, X_results_right))
            return false;

        //合并
        optimize::g_demand.clear();
        for (auto &X : X_results_left)
        {
            optimize::g_demand.demand.push_back(global::g_demand.demand[global::g_demand.get(X.mtime)]);
            optimize::g_demand.mtime.push_back(global::g_demand.mtime[global::g_demand.get(X.mtime)]);
            X_results.push_back(X);
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
        optimize::g_demand.clear();
        for (int i = left; i <= right; i++)
        {
            optimize::g_demand.demand.push_back(global::g_demand.demand[i]);
            optimize::g_demand.mtime.push_back(global::g_demand.mtime[i]);
        }

        if (optimize::solve(NUM_ITERATION, true, X_results) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

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
bool task(const int left, const int right, const int num_iteration, std::vector<ANSWER> &X_results)
{
    optimize::g_demand.clear();
    for (int i = left; i <= right; i++)
    {
        optimize::g_demand.demand.push_back(global::g_demand.demand[i]);
        optimize::g_demand.mtime.push_back(global::g_demand.mtime[i]);
    }

    if (optimize::solve(num_iteration, true, X_results) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int main()
{
    MyUtils::MyTimer::ProcessTimer timer;

    g_qos_constraint = read_qos_constraint();
    read_demand(global::g_demand);
    read_qos(g_qos);
    read_site_bandwidth(g_site_bandwidth);

    std::vector<ANSWER> X_results;

    /*
        {
            std::vector<std::vector<ANSWER>> X_results_vec;
            for (int i = 0; i < global::g_demand.demand.size(); i += NUM_MINIUM)
            {
                X_results_vec.push_back(std::vector<ANSWER>());
                auto &X_results_tmp = X_results_vec.back();
                if (!task(i, i + NUM_MINIUM - 1, NUM_ITERATION, X_results_tmp))
                {
                    std::cout << "error" << std::endl;
                    return 0;
                }
            }

            int num_iteration = NUM_ITERATION;
            auto fun = []() {};
            while (X_results_vec.size() > 1)
            {
                std::vector<std::vector<ANSWER>> X_results_vec_tmp;
                num_iteration *= 2;
                for (int i = 0; i < X_results_vec.size() - 1; i += 2)
                {
                    X_results_vec_tmp.push_back(std::vector<ANSWER>());
                    auto &X_results_tmp = X_results_vec_tmp.back();
                    if (!task(i, i + 1, num_iteration, X_results_tmp))
                    {
                        std::cout << "error" << std::endl;
                        return 0;
                    }
                }
                if (X_results_vec.size() % 2 == 1)
                {
                    X_results_vec_tmp.push_back(std::vector<ANSWER>());
                    auto &X_results_tmp = X_results_vec_tmp.back();
                    if (!task(X_results_vec.size() - 1, X_results_vec.size() - 1, 200, X_results_tmp))
                    {
                        std::cout << "error" << std::endl;
                        return 0;
                    }
                }
                X_results_vec = X_results_vec_tmp;
            }

            optimize::g_demand.clear();
            optimize::g_demand.demand = global::g_demand.demand;
            optimize::g_demand.mtime = global::g_demand.mtime;

            optimize::solve(1000, false, X_results);

        }
        //*/

    {
        divide_conquer(0, global::g_demand.demand.size() - 1, X_results);
    }

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
