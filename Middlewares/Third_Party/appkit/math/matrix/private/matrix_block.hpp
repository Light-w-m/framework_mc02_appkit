#pragma once

#include "matrix_def.hpp"

namespace appkit::math
{
    template <MatrixLike Mat, std::size_t BlockRows, std::size_t BlockCols>
    class MatrixBlock
    {
    public:
        using value_type = typename Mat::value_type;

        static_assert(BlockRows > 0 && BlockCols > 0, "Block dimensions must be greater than zero");
        static_assert(Mat::rows() >= BlockRows && Mat::cols() >= BlockCols, "Block dimensions must be less than or equal to matrix dimensions");

        APPKIT_MATRIX_OPT constexpr MatrixBlock(Mat &matrix, std::size_t start_row = 0, std::size_t start_col = 0) noexcept
            : matrix_(matrix), start_row_(start_row), start_col_(start_col)
        {
        }

        template <MatrixLike InnerMat, std::size_t OtherRows, std::size_t OtherCols>
            requires std::is_same_v<std::remove_cvref_t<Mat>, std::remove_cvref_t<InnerMat>>
        APPKIT_MATRIX_OPT constexpr MatrixBlock(MatrixBlock<InnerMat, OtherRows, OtherCols> &other, std::size_t row_offset = 0, std::size_t col_offset = 0) noexcept
            : matrix_(other.matrix_), start_row_(other.start_row_ + row_offset), start_col_(other.start_col_ + col_offset)
        {
            static_assert(OtherRows >= BlockRows && OtherCols >= BlockCols, "Inner block dimensions must be less than or equal to outer block dimensions");
        }

        template <MatrixLike InnerMat, std::size_t OtherRows, std::size_t OtherCols>
            requires std::is_same_v<std::remove_cvref_t<Mat>, std::remove_cvref_t<InnerMat>>
        APPKIT_MATRIX_OPT constexpr MatrixBlock(const MatrixBlock<InnerMat, OtherRows, OtherCols> &other, std::size_t row_offset = 0, std::size_t col_offset = 0) noexcept
            : matrix_(other.matrix_), start_row_(other.start_row_ + row_offset), start_col_(other.start_col_ + col_offset)
        {
            static_assert(OtherRows >= BlockRows && OtherCols >= BlockCols, "Inner block dimensions must be less than or equal to outer block dimensions");
        }

        [[nodiscard]] APPKIT_MATRIX_OPT constexpr value_type &operator()(std::size_t i, std::size_t j) noexcept
        {
            static_assert(std::is_const_v<Mat> == false, "Cannot modify elements of a const matrix block");
            return matrix_(start_row_ + i, start_col_ + j);
        }

        [[nodiscard]] APPKIT_MATRIX_OPT constexpr value_type operator()(std::size_t i, std::size_t j) const noexcept
        {
            return matrix_(start_row_ + i, start_col_ + j);
        }

        [[nodiscard]] APPKIT_MATRIX_OPT constexpr auto row_data(std::size_t i) noexcept -> value_type *
            requires(!std::is_const_v<Mat> && requires(Mat &m) { m.row_data(0); })
        {
            return matrix_.row_data(start_row_ + i) + start_col_;
        }

        [[nodiscard]] APPKIT_MATRIX_OPT constexpr auto row_data(std::size_t i) const noexcept -> const value_type *
            requires(requires(const Mat &m) { m.row_data(0); })
        {
            return matrix_.row_data(start_row_ + i) + start_col_;
        }

        template <MatrixLike OtherMat>
        APPKIT_MATRIX_OPT constexpr MatrixBlock &operator=(const OtherMat &other) noexcept
        {
            static_assert(OtherMat::rows() == BlockRows && OtherMat::cols() == BlockCols,
                          "Block dimensions must match");

            if constexpr (requires(MatrixBlock &m) { m.row_data(0); } && requires(const OtherMat &m) { m.row_data(0); })
            {
                for (std::size_t i = 0; i < BlockRows; ++i)
                {
                    value_type *dst = row_data(i);
                    const value_type *src = other.row_data(i);
                    for (std::size_t j = 0; j < BlockCols; ++j)
                        dst[j] = src[j];
                }
            }
            else
            {
                for (std::size_t i = 0; i < BlockRows; ++i)
                    for (std::size_t j = 0; j < BlockCols; ++j)
                        operator()(i, j) = other(i, j);
            }

            return *this;
        }

        template <MatrixLike OutMat>
        APPKIT_MATRIX_OPT constexpr void eval_to(OutMat &out) const noexcept
        {
            static_assert(out.rows() >= BlockRows && out.cols() >= BlockCols, "Output matrix dimensions must be greater than or equal to block dimensions");
            static_assert(std::is_same_v<typename OutMat::value_type, value_type>, "Matrix value type must match scalar type");

            if constexpr (requires(const MatrixBlock &m) { m.row_data(0); } && requires(OutMat &m) { m.row_data(0); })
            {
                for (std::size_t i = 0; i < BlockRows; ++i)
                {
                    const value_type *src = row_data(i);
                    value_type *dst = out.row_data(i);
                    for (std::size_t j = 0; j < BlockCols; ++j)
                        dst[j] = src[j];
                }
            }
            else
            {
                for (std::size_t i = 0; i < BlockRows; ++i)
                    for (std::size_t j = 0; j < BlockCols; ++j)
                        out(i, j) = operator()(i, j);
            }
        }

        static consteval std::size_t rows() noexcept { return BlockRows; }
        static consteval std::size_t cols() noexcept { return BlockCols; }

        Mat &matrix_;
        std::size_t start_row_ = 0;
        std::size_t start_col_ = 0;
    };

    template <std::size_t BlockRows, std::size_t BlockCols, std::size_t StartRow, std::size_t StartCol, MatrixLike Mat>
    APPKIT_MATRIX_OPT constexpr MatrixBlock<Mat, BlockRows, BlockCols> make_block(Mat &matrix) noexcept
    {
        static_assert(StartRow + BlockRows <= matrix.rows(), "Block exceeds matrix row bounds");
        static_assert(StartCol + BlockCols <= matrix.cols(), "Block exceeds matrix column bounds");

        return MatrixBlock<Mat, BlockRows, BlockCols>(matrix, StartRow, StartCol);
    }

    template <std::size_t BlockRows, std::size_t BlockCols, std::size_t StartRow, std::size_t StartCol, MatrixLike Mat>
    APPKIT_MATRIX_OPT constexpr MatrixBlock<const Mat, BlockRows, BlockCols> make_block(const Mat &matrix) noexcept
    {
        static_assert(StartRow + BlockRows <= matrix.rows(), "Block exceeds matrix row bounds");
        static_assert(StartCol + BlockCols <= matrix.cols(), "Block exceeds matrix column bounds");

        return MatrixBlock<const Mat, BlockRows, BlockCols>(matrix, StartRow, StartCol);
    }

    template <std::size_t BlockRows, std::size_t BlockCols, std::size_t StartRow, std::size_t StartCol, MatrixLike InnerMat, std::size_t OtherRows, std::size_t OtherCols>
    APPKIT_MATRIX_OPT constexpr MatrixBlock<InnerMat, BlockRows, BlockCols> make_block(MatrixBlock<InnerMat, OtherRows, OtherCols> &outer_block) noexcept
    {
        static_assert(StartRow + BlockRows <= outer_block.rows(), "Block exceeds outer block row bounds");
        static_assert(StartCol + BlockCols <= outer_block.cols(), "Block exceeds outer block column bounds");

        return MatrixBlock<InnerMat, BlockRows, BlockCols>(outer_block.matrix_, StartRow + outer_block.start_row_, StartCol + outer_block.start_col_);
    }

    template <std::size_t BlockRows, std::size_t BlockCols, std::size_t StartRow, std::size_t StartCol, MatrixLike InnerMat, std::size_t OtherRows, std::size_t OtherCols>
    APPKIT_MATRIX_OPT constexpr MatrixBlock<const InnerMat, BlockRows, BlockCols> make_block(const MatrixBlock<InnerMat, OtherRows, OtherCols> &outer_block) noexcept
    {
        static_assert(StartRow + BlockRows <= outer_block.rows(), "Block exceeds outer block row bounds");
        static_assert(StartCol + BlockCols <= outer_block.cols(), "Block exceeds outer block column bounds");

        return MatrixBlock<const InnerMat, BlockRows, BlockCols>(outer_block.matrix_, StartRow + outer_block.start_row_, StartCol + outer_block.start_col_);
    }
} // namespace appkit::math
