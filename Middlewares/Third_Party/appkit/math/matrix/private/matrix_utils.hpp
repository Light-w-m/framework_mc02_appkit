#pragma once

#include "matrix_def.hpp"
#include "matrix_base.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace appkit::math
{
    template <typename T>
        requires std::is_arithmetic_v<T>
    constexpr T ZERO = static_cast<T>(0);

    template <typename T>
        requires std::is_arithmetic_v<T>
    constexpr T ONE = static_cast<T>(1);

    /**
     * @brief 获取矩阵元素的结构化绑定支持
     * @tparam Index 元素索引
     * @tparam Mat 矩阵类型
     * @param mat 矩阵对象
     * @return 元素引用
     */
    template <size_t Index, MatrixLike Mat>
    constexpr auto &get(Mat &mat) noexcept
    {
        static_assert(Index < std::tuple_size<Mat>::value, "Index out of bounds for Vector");
        return mat(Index, 0);
    }

    /**
     * @brief 获取矩阵元素的结构化绑定支持（常量版本）
     * @tparam Index 元素索引
     * @tparam Mat 矩阵类型
     * @param mat 矩阵对象
     * @return 元素常量引用
     */
    template <size_t Index, MatrixLike Mat>
    constexpr const auto &get(const Mat &mat) noexcept
    {
        static_assert(Index < std::tuple_size<Mat>::value, "Index out of bounds for Vector");
        return mat(Index, 0);
    }

    /**
     * @brief 返回一个具体的矩阵对象
     * @tparam Mat 矩阵类型
     * @param matrix 矩阵对象
     * @return 强制计算后的矩阵对象
     */
    template <MatrixLike Mat>
    constexpr auto eval(const Mat &matrix) noexcept -> Matrix<typename Mat::value_type, Mat::rows(), Mat::cols()>
    {
        return {matrix};
    }

    /**
     * @brief 将矩阵元素设置为指定值
     * @tparam Mat 矩阵类型
     * @param matrix 矩阵对象
     * @param value 值
     */
    template <MatrixLike Mat>
    constexpr void fill(Mat &matrix, const typename Mat::value_type &value) noexcept
    {
        for (std::size_t i = 0; i < Mat::rows(); ++i)
            for (std::size_t j = 0; j < Mat::cols(); ++j)
                matrix(i, j) = value;
    }

    /**
     * @brief 创建一个全零矩阵
     * @tparam T 矩阵元素类型
     * @tparam Rows 矩阵行数
     * @tparam Cols 矩阵列数
     * @return 全零矩阵对象
     */
    template <typename T, std::size_t Rows, std::size_t Cols>
    consteval Matrix<T, Rows, Cols> zero_matrix() noexcept
    {
        return Matrix<T, Rows, Cols>{};
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    constexpr Matrix<T, Rows, Cols> constant_matrix(const T &value) noexcept
    {
        Matrix<T, Rows, Cols> result{};
        fill(result, value);
        return result;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    constexpr Matrix<T, Rows, Cols> ones_matrix() noexcept
    {
        return constant_matrix<T, Rows, Cols>(ONE<T>);
    }

    template <typename T, std::size_t Dim>
    constexpr Matrix<T, Dim, 1> unit_vector(std::size_t index) noexcept
    {
        Matrix<T, Dim, 1> result{};
        if (index < Dim)
            result(index, 0) = ONE<T>;
        return result;
    }

    template <typename T, std::size_t Dim, std::convertible_to<T>... Args>
    constexpr Matrix<T, Dim, Dim> diagonal_matrix(Args &&...args) noexcept
    {
        static_assert(sizeof...(Args) == Dim, "Number of arguments must match matrix dimension");
        Matrix<T, Dim, Dim> result{};
        T values[] = {static_cast<T>(args)...};
        for (std::size_t i = 0; i < Dim; ++i)
            result(i, i) = values[i];
        return result;
    }

    template <typename T, std::size_t Dim, MatrixLike Mat>
    constexpr Matrix<T, Dim, Dim> diagonal_matrix(const Mat &matrix) noexcept
    {
        static_assert(Mat::rows() == Dim && Mat::cols() == 1, "Matrix must be a column vector with matching dimension");
        Matrix<T, Dim, Dim> result{};
        for (std::size_t i = 0; i < Dim; ++i)
            result(i, i) = matrix(i, 0);
        return result;
    }

    template <typename T, std::size_t Dim>
    consteval Matrix<T, Dim, Dim> identity_matrix() noexcept
    {
        Matrix<T, Dim, Dim> result{};
        for (std::size_t i = 0; i < Dim; ++i)
            result(i, i) = ONE<T>;
        return result;
    }

    template <MatrixLike Mat>
    constexpr auto trace(const Mat &matrix) noexcept -> typename Mat::value_type
    {
        constexpr std::size_t min_dim = std::min(Mat::rows(), Mat::cols());

        typename Mat::value_type sum = ZERO<typename Mat::value_type>;
        for (std::size_t i = 0; i < min_dim; ++i)
            sum += matrix(i, i);
        return sum;
    }

    template <MatrixLike MatA, MatrixLike MatB>
    constexpr Matrix<typename MatA::value_type, 3, 1> cross(const MatA &matrix_a, const MatB &matrix_b) noexcept
    {
        static_assert(MatA::rows() == 3 && MatA::cols() == 1 &&
                          MatB::rows() == 3 && MatB::cols() == 1,
                      "Cross product is only defined for 3D column vectors.");

        const auto [a1, a2, a3] = matrix_a;
        const auto [b1, b2, b3] = matrix_b;

        return Matrix<typename MatA::value_type, 3, 1>(
            a2 * b3 - a3 * b2,
            a3 * b1 - a1 * b3,
            a1 * b2 - a2 * b1);
    }

    template <MatrixLike MatA, MatrixLike MatB>
    constexpr auto dot(const MatA &matrix_a, const MatB &matrix_b) noexcept -> typename MatA::value_type
    {
        static_assert(MatA::cols() == 1 && MatB::cols() == 1, "Dot product requires column vectors");
        static_assert(MatA::rows() == MatB::rows(), "Vector dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>, "Vector value types must match");

        typename MatA::value_type result{};
        for (std::size_t i = 0; i < MatA::rows(); ++i)
            result += matrix_a(i, 0) * matrix_b(i, 0);

        return result;
    }

    template <MatrixLike MatA, MatrixLike MatB>
    constexpr auto outer(const MatA &matrix_a, const MatB &matrix_b) noexcept -> Matrix<typename MatA::value_type, MatA::rows(), MatB::rows()>
    {
        static_assert(MatA::cols() == 1 && MatB::cols() == 1, "Outer product requires column vectors");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>, "Vector value types must match");

        Matrix<typename MatA::value_type, MatA::rows(), MatB::rows()> result{};
        for (std::size_t i = 0; i < MatA::rows(); ++i)
            for (std::size_t j = 0; j < MatB::rows(); ++j)
                result(i, j) = matrix_a(i, 0) * matrix_b(j, 0);

        return result;
    }

    template <MatrixLike MatA, MatrixLike MatB, MatrixLike MatC>
    constexpr void hadamard_to(const MatA &matrix_a, const MatB &matrix_b, MatC &result) noexcept
    {
        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Matrix dimensions must match");
        static_assert(MatA::rows() == MatC::rows() && MatA::cols() == MatC::cols(), "Result matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type> &&
                          std::is_same_v<typename MatA::value_type, typename MatC::value_type>,
                      "Matrix value types must match");

        for (std::size_t i = 0; i < MatA::rows(); ++i)
            for (std::size_t j = 0; j < MatA::cols(); ++j)
                result(i, j) = matrix_a(i, j) * matrix_b(i, j);
    }

    template <MatrixLike MatA, MatrixLike MatB>
    constexpr auto hadamard(const MatA &matrix_a, const MatB &matrix_b) noexcept -> Matrix<typename MatA::value_type, MatA::rows(), MatA::cols()>
    {
        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>, "Matrix value types must match");

        Matrix<typename MatA::value_type, MatA::rows(), MatA::cols()> result{};
        hadamard_to(matrix_a, matrix_b, result);
        return result;
    }

    template <MatrixLike MatA, MatrixLike MatB, MatrixLike MatC>
    constexpr void cwise_div_to(const MatA &matrix_a, const MatB &matrix_b, MatC &result) noexcept
    {
        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Matrix dimensions must match");
        static_assert(MatA::rows() == MatC::rows() && MatA::cols() == MatC::cols(), "Result matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type> &&
                          std::is_same_v<typename MatA::value_type, typename MatC::value_type>,
                      "Matrix value types must match");

        for (std::size_t i = 0; i < MatA::rows(); ++i)
            for (std::size_t j = 0; j < MatA::cols(); ++j)
                result(i, j) = matrix_a(i, j) / matrix_b(i, j);
    }

    template <MatrixLike MatA, MatrixLike MatB>
    constexpr auto cwise_div(const MatA &matrix_a, const MatB &matrix_b) noexcept -> Matrix<typename MatA::value_type, MatA::rows(), MatA::cols()>
    {
        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>, "Matrix value types must match");

        Matrix<typename MatA::value_type, MatA::rows(), MatA::cols()> result{};
        cwise_div_to(matrix_a, matrix_b, result);
        return result;
    }

    template <MatrixLike Mat>
    constexpr auto cwise_abs(const Mat &matrix) noexcept -> Matrix<typename Mat::value_type, Mat::rows(), Mat::cols()>
    {
        Matrix<typename Mat::value_type, Mat::rows(), Mat::cols()> result{};
        for (std::size_t i = 0; i < Mat::rows(); ++i)
            for (std::size_t j = 0; j < Mat::cols(); ++j)
                result(i, j) = std::abs(matrix(i, j));
        return result;
    }

    template <MatrixLike Mat>
    constexpr auto cwise_clamp(const Mat &matrix,
                               const typename Mat::value_type &min_value,
                               const typename Mat::value_type &max_value) noexcept -> Matrix<typename Mat::value_type, Mat::rows(), Mat::cols()>
    {
        Matrix<typename Mat::value_type, Mat::rows(), Mat::cols()> result{};
        for (std::size_t i = 0; i < Mat::rows(); ++i)
            for (std::size_t j = 0; j < Mat::cols(); ++j)
                result(i, j) = std::clamp(matrix(i, j), min_value, max_value);
        return result;
    }

    template <MatrixLike MatA, MatrixLike MatB>
    constexpr bool is_approx(const MatA &matrix_a,
                             const MatB &matrix_b,
                             typename MatA::value_type epsilon = std::numeric_limits<typename MatA::value_type>::epsilon() * static_cast<typename MatA::value_type>(16)) noexcept
    {
        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>, "Matrix value types must match");

        for (std::size_t i = 0; i < MatA::rows(); ++i)
            for (std::size_t j = 0; j < MatA::cols(); ++j)
                if (std::abs(matrix_a(i, j) - matrix_b(i, j)) > epsilon)
                    return false;

        return true;
    }

    template <MatrixLike MatLeft, MatrixLike MatRight, MatrixLike MatOut>
    constexpr void hstack_to(const MatLeft &left, const MatRight &right, MatOut &out) noexcept
    {
        static_assert(MatLeft::rows() == MatRight::rows(), "Row count must match for horizontal stack");
        static_assert(MatOut::rows() == MatLeft::rows() && MatOut::cols() == MatLeft::cols() + MatRight::cols(),
                      "Output matrix dimensions must match horizontal stack result");
        static_assert(std::is_same_v<typename MatLeft::value_type, typename MatRight::value_type> &&
                          std::is_same_v<typename MatLeft::value_type, typename MatOut::value_type>,
                      "Matrix value types must match");

        for (std::size_t i = 0; i < MatLeft::rows(); ++i)
        {
            for (std::size_t j = 0; j < MatLeft::cols(); ++j)
                out(i, j) = left(i, j);
            for (std::size_t j = 0; j < MatRight::cols(); ++j)
                out(i, MatLeft::cols() + j) = right(i, j);
        }
    }

    template <MatrixLike MatLeft, MatrixLike MatRight>
    constexpr auto hstack(const MatLeft &left, const MatRight &right) noexcept
        -> Matrix<typename MatLeft::value_type, MatLeft::rows(), MatLeft::cols() + MatRight::cols()>
    {
        static_assert(MatLeft::rows() == MatRight::rows(), "Row count must match for horizontal stack");
        static_assert(std::is_same_v<typename MatLeft::value_type, typename MatRight::value_type>, "Matrix value types must match");

        Matrix<typename MatLeft::value_type, MatLeft::rows(), MatLeft::cols() + MatRight::cols()> result{};
        hstack_to(left, right, result);
        return result;
    }

    template <MatrixLike MatTop, MatrixLike MatBottom, MatrixLike MatOut>
    constexpr void vstack_to(const MatTop &top, const MatBottom &bottom, MatOut &out) noexcept
    {
        static_assert(MatTop::cols() == MatBottom::cols(), "Column count must match for vertical stack");
        static_assert(MatOut::rows() == MatTop::rows() + MatBottom::rows() && MatOut::cols() == MatTop::cols(),
                      "Output matrix dimensions must match vertical stack result");
        static_assert(std::is_same_v<typename MatTop::value_type, typename MatBottom::value_type> &&
                          std::is_same_v<typename MatTop::value_type, typename MatOut::value_type>,
                      "Matrix value types must match");

        for (std::size_t i = 0; i < MatTop::rows(); ++i)
            for (std::size_t j = 0; j < MatTop::cols(); ++j)
                out(i, j) = top(i, j);

        for (std::size_t i = 0; i < MatBottom::rows(); ++i)
            for (std::size_t j = 0; j < MatBottom::cols(); ++j)
                out(MatTop::rows() + i, j) = bottom(i, j);
    }

    template <MatrixLike MatTop, MatrixLike MatBottom>
    constexpr auto vstack(const MatTop &top, const MatBottom &bottom) noexcept
        -> Matrix<typename MatTop::value_type, MatTop::rows() + MatBottom::rows(), MatTop::cols()>
    {
        static_assert(MatTop::cols() == MatBottom::cols(), "Column count must match for vertical stack");
        static_assert(std::is_same_v<typename MatTop::value_type, typename MatBottom::value_type>, "Matrix value types must match");

        Matrix<typename MatTop::value_type, MatTop::rows() + MatBottom::rows(), MatTop::cols()> result{};
        vstack_to(top, bottom, result);
        return result;
    }
} // namespace appkit::math

namespace std
{
    template <appkit::math::MatrixLike Mat>
    struct tuple_size<Mat> : std::integral_constant<std::size_t, Mat::rows()>
    {
        static_assert(Mat::cols() == 1, "tuple_size is only defined for column vectors");
    };

    template <size_t Index, appkit::math::MatrixLike Mat>
    struct tuple_element<Index, Mat>
    {
        using type = typename Mat::value_type;
    };
} // namespace std