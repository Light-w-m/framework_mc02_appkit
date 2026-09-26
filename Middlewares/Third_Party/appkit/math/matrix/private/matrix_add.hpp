#pragma once

#include "matrix_def.hpp"

#if APPKIT_MATRIX_USE_PROXY != 1
#include "matrix_base.hpp"
#endif // APPKIT_MATRIX_USE_PROXY

namespace appkit::math
{
#if APPKIT_MATRIX_USE_PROXY == 1
    template <MatrixLike MatA, MatrixLike MatB>
    class MatrixAdd
    {
    public:
        using value_type = typename MatA::value_type;

        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>, "Matrix value types must match");

        static constexpr std::size_t rows() noexcept { return MatA::rows(); }
        static constexpr std::size_t cols() noexcept { return MatA::cols(); }

        APPKIT_MATRIX_OPT constexpr MatrixAdd(const MatA &matrix_a, const MatB &matrix_b) noexcept : matrix_a_(matrix_a), matrix_b_(matrix_b) {}

        [[nodiscard]] APPKIT_MATRIX_OPT constexpr value_type operator()(std::size_t i, std::size_t j) const noexcept
        {
            return matrix_a_(i, j) + matrix_b_(i, j);
        }

        template <MatrixLike OutMat, std::size_t BlockRows = rows(), std::size_t BlockCols = cols()>
        APPKIT_MATRIX_OPT constexpr void eval_to(OutMat &out, std::size_t start_row = 0, std::size_t start_col = 0) const noexcept
        {
            static_assert(BlockRows <= rows() && BlockCols <= cols(), "Block dimensions must be less than or equal to matrix dimensions");
            static_assert(OutMat::rows() == BlockRows && OutMat::cols() == BlockCols, "Result matrix dimensions must match");
            static_assert(std::is_same_v<typename OutMat::value_type, value_type>, "Matrix value types must match");

            if constexpr (requires(const MatA &m) { m.row_data(0); } && requires(const MatB &m) { m.row_data(0); } && requires(OutMat &m) { m.row_data(0); })
            {
                for (std::size_t i = start_row; i < BlockRows; ++i)
                {
                    const value_type *row_a = matrix_a_.row_data(i);
                    const value_type *row_b = matrix_b_.row_data(i);
                    value_type *row_out = out.row_data(i);
                    for (std::size_t j = start_col; j < BlockCols; ++j)
                        row_out[j] = row_a[j] + row_b[j];
                }
            }
            else
            {
                for (std::size_t i = start_row; i < BlockRows; ++i)
                    for (std::size_t j = start_col; j < BlockCols; ++j)
                        out(i, j) = matrix_a_(i, j) + matrix_b_(i, j);
            }
        }

        const MatA &matrix_a_;
        const MatB &matrix_b_;
    };

    template <MatrixLike MatA, MatrixLike MatB, MatrixLike MatC>
    APPKIT_MATRIX_OPT constexpr void add_to(const MatA &matrix_a, const MatB &matrix_b, MatC &result) noexcept
    {
        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Matrix dimensions must match");
        static_assert(MatA::rows() == MatC::rows() && MatA::cols() == MatC::cols(), "Result matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type> &&
                          std::is_same_v<typename MatA::value_type, typename MatC::value_type>,
                      "Matrix value types must match");

        MatrixAdd(matrix_a, matrix_b).eval_to(result);
    }

    template <MatrixLike MatA, MatrixLike MatB>
    APPKIT_MATRIX_OPT constexpr auto operator+(const MatA &matrix_a, const MatB &matrix_b) noexcept
    {
        return MatrixAdd(matrix_a, matrix_b);
    }
#else
    template <MatrixLike MatA, MatrixLike MatB, MatrixLike MatC>
    APPKIT_MATRIX_OPT constexpr void add_to(const MatA &matrix_a, const MatB &matrix_b, MatC &result) noexcept
    {
        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Matrix dimensions must match");
        static_assert(MatA::rows() == MatC::rows() && MatA::cols() == MatC::cols(), "Result matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type> &&
                          std::is_same_v<typename MatA::value_type, typename MatC::value_type>,
                      "Matrix value types must match");

        for (std::size_t i = 0; i < MatA::rows(); ++i)
            for (std::size_t j = 0; j < MatA::cols(); ++j)
                result(i, j) = matrix_a(i, j) + matrix_b(i, j);
    }

    template <MatrixLike MatA, MatrixLike MatB>
    APPKIT_MATRIX_OPT constexpr auto operator+(const MatA &matrix_a, const MatB &matrix_b) noexcept
    {
        static_assert(MatA::rows() == MatB::rows() && MatA::cols() == MatB::cols(), "Matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>, "Matrix value types must match");

        using ResultType = Matrix<typename MatA::value_type, MatA::rows(), MatA::cols()>;
        ResultType result;
        add_to(matrix_a, matrix_b, result);
        return result;
    }
#endif // APPKIT_MATRIX_USE_PROXY

    template <MatrixLike MatA, MatrixLike MatB>
    APPKIT_MATRIX_OPT constexpr MatA &operator+=(MatA &matrix_a, const MatB &matrix_b) noexcept
    {
        add_to(matrix_a, matrix_b, matrix_a);
        return matrix_a;
    }

    template <MatrixLike MatA, MatrixLike MatB>
    APPKIT_MATRIX_OPT constexpr MatA &&operator+=(MatA &&matrix_a, const MatB &matrix_b) noexcept
    {
        add_to(matrix_a, matrix_b, matrix_a);
        return std::move(matrix_a);
    }
} // namespace appkit::math
