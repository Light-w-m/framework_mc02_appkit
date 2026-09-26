#pragma once

#include "matrix_def.hpp"

#if APPKIT_MATRIX_USE_PROXY != 1
#include "matrix_base.hpp"
#endif // APPKIT_MATRIX_USE_PROXY

namespace appkit::math
{
#if APPKIT_MATRIX_USE_PROXY == 1
    template <MatrixLike Mat>
    class MatrixTranspose
    {
    public:
        using value_type = typename Mat::value_type;

        static_assert(Mat::rows() > 0 && Mat::cols() > 0, "Matrix dimensions must be greater than zero");

        APPKIT_MATRIX_OPT constexpr MatrixTranspose(const Mat &matrix) noexcept : matrix_(matrix) {}

        [[nodiscard]] APPKIT_MATRIX_OPT constexpr value_type operator()(std::size_t i, std::size_t j) const noexcept
        {
            return matrix_(j, i);
        }

        static constexpr std::size_t rows() noexcept { return Mat::cols(); }
        static constexpr std::size_t cols() noexcept { return Mat::rows(); }

        const Mat &matrix_;
    };

    template <MatrixLike Mat, MatrixLike TransMat>
    APPKIT_MATRIX_OPT constexpr void transpose_to(const Mat &matrix, TransMat &result) noexcept
    {
        static_assert(Mat::rows() == TransMat::cols() && Mat::cols() == TransMat::rows(), "Transposed matrix dimensions must be swapped");
        static_assert(std::is_same_v<typename Mat::value_type, typename TransMat::value_type>, "Matrix value types must match");

        result = MatrixTranspose(matrix);
    }

    template <MatrixLike Mat>
    APPKIT_MATRIX_OPT constexpr auto transpose(const Mat &matrix) noexcept
    {
        return MatrixTranspose(matrix);
    }
#else
    template <MatrixLike Mat, MatrixLike TransMat>
    APPKIT_MATRIX_OPT constexpr void transpose_to(const Mat &matrix, TransMat &result) noexcept
    {
        static_assert(Mat::rows() == TransMat::cols() && Mat::cols() == TransMat::rows(), "Transposed matrix dimensions must be swapped");
        static_assert(std::is_same_v<typename Mat::value_type, typename TransMat::value_type>, "Matrix value types must match");

        for (std::size_t i = 0; i < Mat::rows(); ++i)
            for (std::size_t j = 0; j < Mat::cols(); ++j)
                result(j, i) = matrix(i, j);
    }

    template <MatrixLike Mat>
    APPKIT_MATRIX_OPT constexpr auto transpose(const Mat &matrix) noexcept
    {
        using ResultType = Matrix<typename Mat::value_type, Mat::cols(), Mat::rows()>;
        ResultType result;
        transpose_to(matrix, result);
        return result;
    }
#endif
} // namespace appkit::math