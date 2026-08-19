// #pragma once

// #include "matrix_def.hpp"

// namespace appkit::math
// {
//     template <MatrixLike MatA>
//     class LUDecomposition
//     {
//     public:
//         using value_type = typename MatA::value_type;

//         static_assert(MatA::rows() == MatA::cols(), "Matrix must be square");

//         constexpr LUDecomposition(const MatA &matrix) noexcept : lu_(matrix)
//         {
//             for (std::size_t k = 0; k < N; ++k)
//             {
//                 std::size_t max_row = k;
//                 value_type lu_kk = lu_(k, k);
//                 value_type max_val = std::abs(lu_kk);
//                 for (std::size_t i = k + 1; i < N; ++i)
//                 {
//                     value_type abs_val = std::abs(lu_(i, k));
//                     if (abs_val > max_val)
//                     {
//                         max_val = abs_val;
//                         max_row = i;
//                     }
//                 }
//                 pivots_[k] = max_row;

//                 if (max_row != k)
//                 {
//                     for (std::size_t j = 0; j < N; ++j)
//                     {
//                         swap(lu_(k, j), lu_(max_row, j));
//                     }
//                 }

//                 if (lu_kk == value_type{0})
//                     continue; // 奇异，继续分解但后续求解会失败

//                 for (std::size_t i = k + 1; i < N; ++i)
//                 {
//                     value_type factor = lu_(i, k) / lu_kk;
//                     lu_(i, k) = factor;
//                     for (std::size_t j = k + 1; j < N; ++j)
//                     {
//                         lu_(i, j) -= factor * lu_(k, j);
//                     }
//                 }
//             }
//         }

//         template <MatrixLike MatB>
//         constexpr bool solve(const MatB &b, MatB &x) const noexcept
//         {
//             // static_assert(MatB::rows() == MatA::rows() && MatB::cols() == 1, "Right-hand side must be a column vector with matching dimension");
//             // static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>, "Matrix value types must match");

//             // // 前向替换 Ly = Pb
//             // for (std::size_t i = 0; i < N; ++i)
//             // {
//             //     x(i, 0) = b(pivots_[i], 0);
//             //     for (std::size_t j = 0; j < i; ++j)
//             //     {
//             //         x(i, 0) -= lu_(i, j) * x(j, 0);
//             //     }
//             // }

//             // // 后向替换 Ux = y
//             // for (std::size_t i = N; i-- > 0;)
//             // {
//             //     if (lu_(i, i) == value_type{0})
//             //         return false; // 奇异，无法求解

//             //     for (std::size_t j = i + 1; j < N; ++j)
//             //     {
//             //         x(i, 0) -= lu_(i, j) * x(j, 0);
//             //     }
//             //     x(i, 0) /= lu_(i, i);
//             // }

//             // return true;
//         }

//     private:
//         MatA lu_;
//         std::size_t pivot_[MatA::rows()]{};
//     };
// } // namespace appkit::math
