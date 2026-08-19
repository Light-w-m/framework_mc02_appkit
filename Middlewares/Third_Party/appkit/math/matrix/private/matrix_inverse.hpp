#pragma once

#include "matrix_def.hpp"
#include "matrix_base.hpp"
#include "matrix_block.hpp"

namespace appkit::math
{
    template <MatrixLike MatA, MatrixLike MatB>
    constexpr bool inv_to(const MatA &matrix_a, MatB &result) noexcept
    {
        static_assert(MatA::rows() == MatA::cols(), "Matrix must be square");
        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Result matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>, "Matrix value types must match");

        if constexpr (MatA::rows() == 1)
        {
            result(0, 0) = ONE<typename MatA::value_type> / matrix_a(0, 0);

            return true;
        }
        else if constexpr (MatA::rows() == 2)
        {
            const typename MatA::value_type a_00 = matrix_a(0, 0), a_01 = matrix_a(0, 1),
                                            a_10 = matrix_a(1, 0), a_11 = matrix_a(1, 1);

            const typename MatA::value_type det = a_00 * a_11 - a_01 * a_10;
            if (det == ZERO<typename MatA::value_type>)
                return false; // 奇异矩阵不可逆

            const typename MatA::value_type inv_det = ONE<typename MatA::value_type> / det;

            result(0, 0) = a_11 * inv_det;
            result(0, 1) = -a_01 * inv_det;
            result(1, 0) = -a_10 * inv_det;
            result(1, 1) = a_00 * inv_det;

            return true;
        }
        else if constexpr (MatA::rows() == 3)
        {
            const typename MatA::value_type a_00 = matrix_a(0, 0), a_01 = matrix_a(0, 1), a_02 = matrix_a(0, 2),
                                   a_10 = matrix_a(1, 0), a_11 = matrix_a(1, 1), a_12 = matrix_a(1, 2),
                                   a_20 = matrix_a(2, 0), a_21 = matrix_a(2, 1), a_22 = matrix_a(2, 2);

            const typename MatA::value_type det = a_00 * (a_11 * a_22 - a_12 * a_21) -
                                         a_01 * (a_10 * a_22 - a_12 * a_20) +
                                         a_02 * (a_10 * a_21 - a_11 * a_20);
            if (det == ZERO<typename MatA::value_type>)
                return false; // 奇异矩阵不可逆

            const typename MatA::value_type inv_det = ONE<typename MatA::value_type> / det;

            result(0, 0) = (a_11 * a_22 - a_12 * a_21) * inv_det;
            result(0, 1) = (a_02 * a_21 - a_01 * a_22) * inv_det;
            result(0, 2) = (a_01 * a_12 - a_02 * a_11) * inv_det;
            result(1, 0) = (a_12 * a_20 - a_10 * a_22) * inv_det;
            result(1, 1) = (a_00 * a_22 - a_02 * a_20) * inv_det;
            result(1, 2) = (a_02 * a_10 - a_00 * a_12) * inv_det;
            result(2, 0) = (a_10 * a_21 - a_11 * a_20) * inv_det;
            result(2, 1) = (a_01 * a_20 - a_00 * a_21) * inv_det;
            result(2, 2) = (a_00 * a_11 - a_01 * a_10) * inv_det;

            return true;
        }
        else
        {
            // 使用列主元高斯消元法计算逆矩阵
            Matrix<typename MatA::value_type, MatA::rows(), MatA::cols() * 2> augmented;
            make_block<MatA::rows(), MatA::cols(), 0, 0>(augmented) = matrix_a;
            make_block<MatA::rows(), MatA::cols(), 0, MatA::cols()>(augmented) = identity_matrix<typename MatA::value_type, MatA::rows()>();

            for (std::size_t i = 0; i < MatA::rows(); ++i)
            {
                // 寻找主元
                std::size_t pivot = i;
                for (std::size_t j = i + 1; j < MatA::rows(); ++j)
                {
                    if (std::abs(augmented(j, i)) > std::abs(augmented(pivot, i)))
                        pivot = j;
                }

                // 如果主元为零，则矩阵不可逆
                if (augmented(pivot, i) == ZERO<typename MatA::value_type>)
                    return false;

                // 交换当前行和主元行
                if (pivot != i)
                {
                    for (std::size_t j = 0; j < 2 * MatA::cols(); ++j)
                        std::swap(augmented(i, j), augmented(pivot, j));
                }

                // 将主元行归一化
                typename MatA::value_type inv_pivot = ONE<typename MatA::value_type> / augmented(i, i);
                for (std::size_t j = 0; j < 2 * MatA::cols(); ++j)
                    augmented(i, j) *= inv_pivot;

                // 消去其他行的当前列
                for (std::size_t j = 0; j < MatA::rows(); ++j)
                {
                    if (j != i)
                    {
                        typename MatA::value_type factor = augmented(j, i);
                        for (std::size_t k = 0; k < 2 * MatA::cols(); ++k)
                            augmented(j, k) -= factor * augmented(i, k);
                    }
                }
            }

            // 提取逆矩阵
            result = make_block<MatA::rows(), MatA::cols(), 0, MatA::cols()>(augmented);

            return true;
        }
    }
} // namespace appkit::math
