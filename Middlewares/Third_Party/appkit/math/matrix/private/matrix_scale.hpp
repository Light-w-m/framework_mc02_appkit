#pragma once

#include "matrix_def.hpp"

#if APPKIT_MATRIX_USE_PROXY != 1
#include "matrix_base.hpp"
#endif // APPKIT_MATRIX_USE_PROXY

namespace appkit::math
{
#if APPKIT_MATRIX_USE_PROXY == 1
    template <MatrixLike Mat>
    class MatrixScale
    {
    public:
        using value_type = typename Mat::value_type;

        static consteval std::size_t rows() noexcept { return Mat::rows(); }
        static consteval std::size_t cols() noexcept { return Mat::cols(); }

        APPKIT_MATRIX_OPT constexpr MatrixScale(const Mat &matrix, std::convertible_to<typename Mat::value_type> auto scalar) noexcept
            : matrix_(matrix), scalar_(scalar) {}

        APPKIT_MATRIX_OPT constexpr MatrixScale(const MatrixScale<Mat> &other, std::convertible_to<typename Mat::value_type> auto scalar) noexcept
            : MatrixScale(other.matrix_, other.scalar_ * scalar)
        {
        }

        [[nodiscard]] APPKIT_MATRIX_OPT constexpr value_type operator()(std::size_t i, std::size_t j) const noexcept
        {
            return matrix_(i, j) * scalar_;
        }

        template <MatrixLike OutMat, std::size_t BlockRows = rows(), std::size_t BlockCols = cols()>
        APPKIT_MATRIX_OPT constexpr void eval_to(OutMat &out, std::size_t start_row = 0, std::size_t start_col = 0) const noexcept
        {
            static_assert(BlockRows <= rows() && BlockCols <= cols(), "Block dimensions must be less than or equal to matrix dimensions");
            static_assert(OutMat::rows() == BlockRows && OutMat::cols() == BlockCols, "Result matrix dimensions must match");
            static_assert(std::is_same_v<typename OutMat::value_type, value_type>, "Matrix value type must match scalar type");

            if constexpr (requires(const Mat &m) { m.row_data(0); } && requires(OutMat &m) { m.row_data(0); })
            {
                for (std::size_t i = start_row; i < BlockRows; ++i)
                {
                    const value_type *src_row = matrix_.row_data(i);
                    value_type *dst_row = out.row_data(i);
                    for (std::size_t j = start_col; j < BlockCols; ++j)
                        dst_row[j] = src_row[j] * scalar_;
                }
            }
            else
            {
                for (std::size_t i = start_row; i < BlockRows; ++i)
                    for (std::size_t j = start_col; j < BlockCols; ++j)
                        out(i, j) = matrix_(i, j) * scalar_;
            }
        }

        const Mat &matrix_;
        const typename Mat::value_type scalar_;
    };

    template <MatrixLike MatA, std::convertible_to<typename MatA::value_type> Scalar, MatrixLike MatB>
    APPKIT_MATRIX_OPT constexpr void mul_scalar_to(const MatA &matrix, const Scalar &scalar, MatB &result) noexcept
    {
        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Result matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>,
                      "Matrix value type must match scalar type");

        MatrixScale(matrix, scalar).eval_to(result);
    }

    template <MatrixLike Mat, std::convertible_to<typename Mat::value_type> Scalar>
    APPKIT_MATRIX_OPT constexpr auto operator*(const Mat &matrix, const Scalar &scalar) noexcept
    {
        return MatrixScale(matrix, scalar);
    }

#else
    template <MatrixLike MatA, std::convertible_to<typename MatA::value_type> Scalar, MatrixLike MatB>
    APPKIT_MATRIX_OPT constexpr void mul_scalar_to(const MatA &matrix, const Scalar &scalar, MatB &result) noexcept
    {
        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Result matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type> &&
                          std::is_same_v<typename MatA::value_type, Scalar>,
                      "Matrix value type must match scalar type");

        for (std::size_t i = 0; i < MatA::rows(); ++i)
            for (std::size_t j = 0; j < MatA::cols(); ++j)
                result(i, j) = matrix(i, j) * scalar;
    }

    template <MatrixLike MatA, std::convertible_to<typename MatA::value_type> Scalar>
    APPKIT_MATRIX_OPT constexpr auto operator*(const MatA &matrix, const Scalar &scalar) noexcept
    {
        using ResultType = Matrix<typename MatA::value_type, MatA::rows(), MatA::cols()>;
        ResultType result;
        mul_scalar_to(matrix, scalar, result);
        return result;
    }

#endif // APPKIT_MATRIX_USE_PROXY

    template <MatrixLike MatA, std::convertible_to<typename MatA::value_type> Scalar>
    APPKIT_MATRIX_OPT constexpr auto operator*(const Scalar &scalar, const MatA &matrix) noexcept
    {
        return matrix * scalar;
    }

    template <MatrixLike MatA, std::convertible_to<typename MatA::value_type> Scalar>
    APPKIT_MATRIX_OPT constexpr MatA &operator*=(MatA &matrix, const Scalar &scalar) noexcept
    {
        mul_scalar_to(matrix, scalar, matrix);
        return matrix;
    }

    template <MatrixLike MatA, std::convertible_to<typename MatA::value_type> Scalar>
    APPKIT_MATRIX_OPT constexpr auto operator/(const MatA &matrix, const Scalar &scalar) noexcept
    {
        return matrix * (1 / scalar);
    }

    template <MatrixLike MatA, std::convertible_to<typename MatA::value_type> Scalar>
    APPKIT_MATRIX_OPT constexpr MatA &operator/=(MatA &matrix, const Scalar &scalar) noexcept
    {
        mul_scalar_to(matrix, 1 / scalar, matrix);
        return matrix;
    }

    template <MatrixLike MatA>
    APPKIT_MATRIX_OPT constexpr auto operator-(const MatA &matrix) noexcept
    {
        return matrix * (-1);
    }
} // namespace appkit::math

#if APPKIT_MATRIX_USE_PROXY == 1
#include <matrix_mul.hpp>

namespace appkit::math
{
    template <MatrixLike MatA, std::convertible_to<typename MatA::value_type> Scalar, MatrixLike MatB>
    APPKIT_MATRIX_OPT constexpr auto operator*(const MatrixScale<MatA> &matrix_a, const MatB &matrix_b) noexcept
    {
        return MatrixScale(MatrixMul(matrix_a.matrix_, matrix_b), matrix_a.scalar_);
    }

    template <MatrixLike MatA, MatrixLike MatB, std::convertible_to<typename MatA::value_type> Scalar>
    APPKIT_MATRIX_OPT constexpr auto operator*(const MatA &matrix_a, const MatrixScale<MatB> &matrix_b) noexcept
    {
        return MatrixScale(MatrixMul(matrix_a, matrix_b.matrix_), matrix_b.scalar_);
    }

    template <MatrixLike MatA, std::convertible_to<typename MatA::value_type> ScalarA, MatrixLike MatB, std::convertible_to<typename MatB::value_type> ScalarB>
    APPKIT_MATRIX_OPT constexpr auto operator*(const MatrixScale<MatA> &matrix_a, const MatrixScale<MatB> &matrix_b) noexcept
    {
        return MatrixScale(MatrixMul(matrix_a.matrix_, matrix_b.matrix_), matrix_a.scalar_ * matrix_b.scalar_);
    }
} // namespace appkit::math
#endif