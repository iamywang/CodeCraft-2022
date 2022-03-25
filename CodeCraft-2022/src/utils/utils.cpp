#include "../global.hpp"
#include <cmath>
#include "../optimize/optimize_interface.hpp"

/**
 * @brief 根据size计算对应百分位值的索引（向上取整）
 *
 * @param quantile
 * @param size
 * @return int
 */
int calculate_quantile_index(double quantile, unsigned long size)
{
    return std::ceil(quantile * size) - 1; //因为是索引，所以需要减去1
}


