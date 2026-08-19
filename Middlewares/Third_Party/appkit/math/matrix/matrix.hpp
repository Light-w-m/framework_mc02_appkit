#pragma once

#include <matrix_def.hpp>
#include <matrix_base.hpp>
#include <matrix_block.hpp>
#include <matrix_utils.hpp>
#include <matrix_norm.hpp>
#include <matrix_add.hpp>
#include <matrix_sub.hpp>
#include <matrix_mul.hpp>
#include <matrix_scale.hpp>
#include <matrix_inverse.hpp>
#include <matrix_transpose.hpp>
#include <matrix_lu.hpp>

namespace appkit::math
{
    template <std::size_t Rows, std::size_t Cols>
    using Matrixf = Matrix<float, Rows, Cols>;

    template <std::size_t Rows, std::size_t Cols>
    using Matrixd = Matrix<double, Rows, Cols>;
    
    template <std::size_t Rows, std::size_t Cols>
    using Matrixi = Matrix<int, Rows, Cols>;

    template <typename T, std::size_t Rows>
    using Vector = Matrix<T, Rows, 1>;

    template <std::size_t Rows>
    using Vectorf = Matrix<float, Rows, 1>;

    template <std::size_t Rows>
    using Vectord = Matrix<double, Rows, 1>;

    template <std::size_t Rows>
    using Vectori = Matrix<int, Rows, 1>;
} // namespace appkit::math
