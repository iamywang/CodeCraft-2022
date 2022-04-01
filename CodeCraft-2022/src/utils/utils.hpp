#pragma once

#include "../global.hpp"
#include <vector>
#include <future>

int calculate_quantile_index(double quantile, unsigned long size);

/**
 * @brief 将当前的for循环的任务划分为不同的线程去执行。
 *

 * @tparam F
 * @tparam Args
 * @param start_index
 * @param end_index
 * @param func
 * @param args
 * @return decltype(f(args...))
 */
template <typename F, class... Args>
auto parallel_for(const int start_index,
                  const int end_index,
                  F &&func,
                  Args &&...args) -> std::vector<std::future<decltype(func(0, args...))>>
{
    using RetType = decltype(func(0, args...));

    int n = g_thread_pool.idlCount();
    const int step = (end_index - start_index) / (n + 1) + 1; // n+1是因为当前线程也可以处理

    std::vector<std::future<RetType>> futures;

    {
        int left = start_index;
        for (; left < end_index && n; left += step, --n)
        {
            futures.push_back(
                g_thread_pool.commit(
                    [&, left, step, end_index]() -> RetType
                    {
                        for (int i = left; i < left + step && i < end_index; i++)
                        {
                            func(i, args...);
                        }
                        return RetType();
                    }));
        }
        for (; left < end_index; left++)
        {
            func(left, args...);
        }
    }

    return futures;
}
