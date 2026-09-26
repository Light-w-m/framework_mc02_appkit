#pragma once

#include "matrix_def.hpp"

namespace appkit::math
{
    template <typename T, std::size_t Rows, std::size_t Cols>
    class Matrix
    {
    public:
        using value_type = T;

        static_assert(Rows > 0 && Cols > 0, "Matrix dimensions must be greater than zero");
        static_assert(std::is_arithmetic_v<T>, "Matrix value type must be arithmetic");

        static constexpr std::size_t rows() noexcept { return Rows; }
        static constexpr std::size_t cols() noexcept { return Cols; }

        [[nodiscard]] APPKIT_MATRIX_OPT constexpr value_type &operator()(std::size_t i, std::size_t j) noexcept { return data_[i][j]; }
        [[nodiscard]] APPKIT_MATRIX_OPT constexpr const value_type &operator()(std::size_t i, std::size_t j) const noexcept { return data_[i][j]; }

        [[nodiscard]] APPKIT_MATRIX_OPT constexpr value_type *row_data(std::size_t i) noexcept { return data_[i]; }
        [[nodiscard]] APPKIT_MATRIX_OPT constexpr const value_type *row_data(std::size_t i) const noexcept { return data_[i]; }
        [[nodiscard]] APPKIT_MATRIX_OPT constexpr value_type *data() noexcept { return &data_[0][0]; }
        [[nodiscard]] APPKIT_MATRIX_OPT constexpr const value_type *data() const noexcept { return &data_[0][0]; }

        APPKIT_MATRIX_OPT constexpr Matrix() noexcept = default;

        template <std::convertible_to<T>... Args>
        APPKIT_MATRIX_OPT constexpr Matrix(Args &&...args) noexcept : data_{static_cast<T>(args)...} {}

        template <MatrixLike Mat>
        APPKIT_MATRIX_OPT constexpr Matrix(const Mat &other) noexcept
        {
            static_assert(Mat::rows() == Rows && Mat::cols() == Cols, "Matrix dimensions must match");
            static_assert(std::is_convertible_v<typename Mat::value_type, value_type>, "Matrix value type must be convertible");

            if constexpr (requires(const Mat &m, Matrix &out) { m.eval_to(out); })
            {
                other.eval_to(*this);
            }
            else if constexpr (requires(const Mat &m) { m.row_data(0); })
            {
                for (std::size_t i = 0; i < Rows; ++i)
                {
                    const value_type *src = other.row_data(i);
                    value_type *dst = row_data(i);
                    for (std::size_t j = 0; j < Cols; ++j)
                        dst[j] = src[j];
                }
            }
            else
            {
                for (std::size_t i = 0; i < Rows; ++i)
                    for (std::size_t j = 0; j < Cols; ++j)
                        data_[i][j] = other(i, j);
            }
        }

        template <MatrixLike Mat>
        APPKIT_MATRIX_OPT constexpr Matrix &operator=(const Mat &other) noexcept
        {
            static_assert(Mat::rows() == Rows && Mat::cols() == Cols, "Matrix dimensions must match");
            static_assert(std::is_convertible_v<typename Mat::value_type, value_type>, "Matrix value type must be convertible");

            (void)new (this) Matrix(other);

            return *this;
        }

        template <MatrixLike Mat, std::size_t BlockRows = Rows, std::size_t BlockCols = Cols>
        APPKIT_MATRIX_OPT constexpr void eval_to(Mat &other, std::size_t start_row = 0, std::size_t start_col = 0) const noexcept
        {
            static_assert(BlockRows <= Rows && BlockCols <= Cols, "Block dimensions must be less than or equal to matrix dimensions");
            static_assert(Mat::rows() == BlockRows && Mat::cols() == BlockCols, "Matrix dimensions must match");
            static_assert(std::is_convertible_v<typename Mat::value_type, value_type>, "Matrix value type must be convertible");

            if constexpr (requires(Mat &m) { m.row_data(0); })
            {
                for (std::size_t i = start_row; i < BlockRows; ++i)
                {
                    const value_type *src = row_data(i);
                    value_type *dst = other.row_data(i);
                    for (std::size_t j = start_col; j < BlockCols; ++j)
                        dst[j] = src[j];
                }
            }
            else
            {
                for (std::size_t i = start_row; i < BlockRows; ++i)
                    for (std::size_t j = start_col; j < BlockCols; ++j)
                        other(i, j) = data_[i][j];
            }
        }


    private:
        alignas(64) value_type data_[Rows][Cols]{};
    };
} // namespace appkit::math
