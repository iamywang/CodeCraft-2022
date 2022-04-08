#pragma once

#include "global.hpp"
#include "solve/Solver.hpp"

const int NUM_MINIUM_PER_BLOCK = 200; //最小分组中每组最多有多少个元素
const int NUM_ITERATION = 200;        //最小分组的最大迭代次数

class DivideConquer
{

public:
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
    static bool task(const int left,
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
    template <bool is_generate_new_results>
    static bool divide_conquer(const int left,
                               const int right,
                               std::vector<ANSWER> &X_results)
    {
        if (right - left > NUM_MINIUM_PER_BLOCK)
        {
            const int mid = (left + right) / 2;

            std::vector<ANSWER> X_results_right;
            if (!is_generate_new_results)
            {
                for (int i = mid + 1 - left; i <= right - left; ++i)
                // for (int i = mid + 1; i <= right; ++i)
                {
                    if (i >= X_results.size())
                    {
                        throw runtime_error("i >= X_results.size()");
                        exit(-1);
                    }
                    X_results_right.push_back(std::move(X_results[i]));
                }
                X_results.resize(mid - left + 1);
            }

            if (!divide_conquer<is_generate_new_results>(left, mid, X_results))
                return false;

            if (!divide_conquer<is_generate_new_results>(mid + 1, right, X_results_right))
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
            return task(left, right, NUM_ITERATION, is_generate_new_results, X_results);
        }
    }
};
