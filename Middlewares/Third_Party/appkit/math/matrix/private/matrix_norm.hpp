#pragma once

#include "matrix_def.hpp"
#include <cmath>

#include <utils.hpp>

namespace appkit::math
{
    enum class NormType
    {
        L1, ///< L1范数，矩阵元素绝对值之和
        L2, ///< L2范数，矩阵元素平方和的平方根
        Inf ///< 无穷范数，矩阵行绝对值之和的最大值
    };

    template <MatrixLike Mat, NormType Type = NormType::L2>
    constexpr auto norm(const Mat &matrix) noexcept -> typename Mat::value_type
    {
        using value_type = typename Mat::value_type;
        value_type result = ZERO<value_type>;

        if constexpr (Type == NormType::L1)
        {
            for (std::size_t j = 0; j < Mat::cols(); ++j)
            {
                for (std::size_t i = 0; i < Mat::rows(); ++i)
                    result += std::abs(matrix(i, j));
            }
        }
        else if constexpr (Type == NormType::L2)
        {
            for (std::size_t i = 0; i < Mat::rows(); ++i)
                for (std::size_t j = 0; j < Mat::cols(); ++j)
                {
                    const auto val = matrix(i, j);
                    result += val * val;
                }

            if constexpr (std::is_same_v<value_type, float>)
                result *= fast::InvSqrt(result);
            else
                result = std::sqrt(result);
        }
        else if constexpr (Type == NormType::Inf)
        {
            value_type row_sum;
            for (std::size_t i = 0; i < Mat::rows(); ++i)
            {
                row_sum = ZERO<value_type>;
                for (std::size_t j = 0; j < Mat::cols(); ++j)
                    row_sum += std::abs(matrix(i, j));
                result = std::max(result, row_sum);
            }
        }

        return result;
    }
} // namespace appkit::math
