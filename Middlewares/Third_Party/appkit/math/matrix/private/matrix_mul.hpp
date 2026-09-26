#pragma once

#include "matrix_def.hpp"
#include "matrix_base.hpp"

#include <array>
#include <limits>
#include <tuple>

namespace appkit::math
{
    namespace detail
    {
        struct EmptyCache
        {
        };

        template <MatrixLike MatA, MatrixLike MatB, MatrixLike MatC>
        APPKIT_MATRIX_OPT constexpr void mul_kernel(const MatA &matrix_a, const MatB &matrix_b, MatC &result) noexcept
        {
            using ValueType = typename MatA::value_type;

            if constexpr (requires(const MatA &m) { m.row_data(0); } && requires(const MatB &m) { m.row_data(0); } && requires(MatC &m) { m.row_data(0); })
            {
                for (std::size_t i = 0; i < MatA::rows(); ++i)
                {
                    ValueType *row_out = result.row_data(i);
                    for (std::size_t j = 0; j < MatB::cols(); ++j)
                        row_out[j] = ValueType{};

                    const ValueType *row_a = matrix_a.row_data(i);
                    for (std::size_t k = 0; k < MatA::cols(); ++k)
                    {
                        const ValueType a_ik = row_a[k];
                        const ValueType *row_b = matrix_b.row_data(k);
                        for (std::size_t j = 0; j < MatB::cols(); ++j)
                            row_out[j] += a_ik * row_b[j];
                    }
                }
            }
            else if constexpr (requires(const MatA &m, Matrix<ValueType, MatA::rows(), MatA::cols()> &tmp) { m.eval_to(tmp); } && requires(const MatB &m) { m.row_data(0); } && requires(MatC &m) { m.row_data(0); })
            {
                Matrix<ValueType, MatA::rows(), MatA::cols()> left{};
                matrix_a.eval_to(left);
                mul_kernel(left, matrix_b, result);
            }
            else if constexpr (requires(const MatB &m, Matrix<ValueType, MatB::rows(), MatB::cols()> &tmp) { m.eval_to(tmp); } && requires(const MatA &m) { m.row_data(0); } && requires(MatC &m) { m.row_data(0); })
            {
                Matrix<ValueType, MatB::rows(), MatB::cols()> right{};
                matrix_b.eval_to(right);
                mul_kernel(matrix_a, right, result);
            }
            else if constexpr (requires(const MatA &m, Matrix<ValueType, MatA::rows(), MatA::cols()> &tmp) { m.eval_to(tmp); } && requires(const MatB &m, Matrix<ValueType, MatB::rows(), MatB::cols()> &tmp) { m.eval_to(tmp); })
            {
                Matrix<ValueType, MatA::rows(), MatA::cols()> left{};
                Matrix<ValueType, MatB::rows(), MatB::cols()> right{};
                matrix_a.eval_to(left);
                matrix_b.eval_to(right);
                mul_kernel(left, right, result);
            }
            else
            {
                for (std::size_t i = 0; i < MatA::rows(); ++i)
                {
                    for (std::size_t j = 0; j < MatB::cols(); ++j)
                        result(i, j) = ValueType{};

                    for (std::size_t k = 0; k < MatA::cols(); ++k)
                    {
                        const ValueType a_ik = matrix_a(i, k);
                        for (std::size_t j = 0; j < MatB::cols(); ++j)
                            result(i, j) += a_ik * matrix_b(k, j);
                    }
                }
            }
        }

        template <typename Tuple, std::size_t... Is>
        consteval bool chain_compatible_impl(std::index_sequence<Is...>)
        {
            return ((std::tuple_element_t<Is, Tuple>::cols() == std::tuple_element_t<Is + 1, Tuple>::rows()) && ...);
        }

        template <typename Tuple>
        consteval bool chain_compatible()
        {
            constexpr std::size_t term_count = std::tuple_size_v<Tuple>;
            if constexpr (term_count <= 1)
                return true;
            else
                return chain_compatible_impl<Tuple>(std::make_index_sequence<term_count - 1>{});
        }

        template <typename Tuple, std::size_t... Is>
        consteval auto chain_dims_impl(std::index_sequence<Is...>)
        {
            return std::array<std::size_t, sizeof...(Is) + 1>{
                std::tuple_element_t<0, Tuple>::rows(),
                std::tuple_element_t<Is, Tuple>::cols()...};
        }

        template <typename Tuple>
        consteval auto chain_dims()
        {
            constexpr std::size_t term_count = std::tuple_size_v<Tuple>;
            static_assert(term_count > 0, "Matrix chain must contain at least one term");
            return chain_dims_impl<Tuple>(std::make_index_sequence<term_count>{});
        }

        template <std::size_t TermCount>
        consteval auto chain_split_plan(const std::array<std::size_t, TermCount + 1> &dims)
        {
            std::array<std::array<std::size_t, TermCount>, TermCount> split{};
            std::array<std::array<std::size_t, TermCount>, TermCount> cost{};

            for (std::size_t len = 2; len <= TermCount; ++len)
            {
                for (std::size_t i = 0; i + len <= TermCount; ++i)
                {
                    const std::size_t j = i + len - 1;
                    std::size_t best_cost = std::numeric_limits<std::size_t>::max();
                    std::size_t best_split = i;

                    for (std::size_t k = i; k < j; ++k)
                    {
                        const std::size_t left_cost = cost[i][k];
                        const std::size_t right_cost = cost[k + 1][j];
                        const std::size_t mul_cost = dims[i] * dims[k + 1] * dims[j + 1];
                        const std::size_t total_cost = left_cost + right_cost + mul_cost;

                        if (total_cost < best_cost)
                        {
                            best_cost = total_cost;
                            best_split = k;
                        }
                    }

                    cost[i][j] = best_cost;
                    split[i][j] = best_split;
                }
            }

            return split;
        }

        template <std::size_t TermCount>
        consteval auto linear_split_plan()
        {
            std::array<std::array<std::size_t, TermCount>, TermCount> split{};

            for (std::size_t i = 0; i < TermCount; ++i)
                for (std::size_t j = i + 1; j < TermCount; ++j)
                    split[i][j] = j - 1;

            return split;
        }
    } // namespace detail

#if APPKIT_MATRIX_USE_PROXY == 1
    template <MatrixLike MatA, MatrixLike MatB, MatrixLike... Rest>
    class MatrixMul
    {
    public:
        using value_type = typename MatA::value_type;
        static constexpr std::size_t term_count = 2 + sizeof...(Rest);
        static constexpr std::size_t dp_min_term_count = 5;
        using mats_tuple = std::tuple<MatA, MatB, Rest...>;

        template <std::size_t Index>
        using term_t = std::tuple_element_t<Index, mats_tuple>;

        static_assert(std::is_same_v<value_type, typename MatB::value_type> && (std::is_same_v<value_type, typename Rest::value_type> && ...),
                      "Matrix value types must match");
        static_assert(detail::chain_compatible<mats_tuple>(), "Inner matrix dimensions must match");

        APPKIT_MATRIX_OPT constexpr MatrixMul(const MatA &matrix_a, const MatB &matrix_b, const Rest &...rest) noexcept
            : matrices_(matrix_a, matrix_b, rest...)
        {
        }

        [[nodiscard]] APPKIT_MATRIX_OPT constexpr value_type operator()(std::size_t i, std::size_t j) const noexcept
        {
            if constexpr (term_count == 2)
            {
                value_type sum = value_type{};
                const auto &matrix_a = std::get<0>(matrices_);
                const auto &matrix_b = std::get<1>(matrices_);
                for (std::size_t k = 0; k < MatA::cols(); ++k)
                    sum += matrix_a(i, k) * matrix_b(k, j);
                return sum;
            }
            else
            {
                if (cache_valid_)
                    return cache_.row_data(i)[j];

                // Scalar access should avoid materializing the entire expression.
                return eval_scalar_segment<0, term_count - 1>(i, j);
            }
        }

        template <MatrixLike OutMat>
        APPKIT_MATRIX_OPT constexpr void eval_to(OutMat &out) const noexcept
        {
            static_assert(OutMat::rows() == rows() && OutMat::cols() == cols(), "Result matrix dimensions must match");
            static_assert(std::is_same_v<typename OutMat::value_type, value_type>, "Matrix value types must match");            eval_segment<0, term_count - 1>(out);
        }

        [[nodiscard]] APPKIT_MATRIX_OPT constexpr auto row_data(std::size_t i) const noexcept -> const value_type *
            requires(term_count > 2)
        {
            ensure_cache();
            return cache_.row_data(i);
        }

        static constexpr std::size_t rows() noexcept { return MatA::rows(); }
        static constexpr std::size_t cols() noexcept { return term_t<term_count - 1>::cols(); }

        template <MatrixLike MatNext>
        [[nodiscard]] APPKIT_MATRIX_OPT constexpr auto append(const MatNext &next) const noexcept
        {
            return append_impl(next, std::make_index_sequence<term_count>{});
        }

        template <MatrixLike MatPrev>
        [[nodiscard]] APPKIT_MATRIX_OPT constexpr auto prepend(const MatPrev &prev) const noexcept
        {
            return prepend_impl(prev, std::make_index_sequence<term_count>{});
        }

        template <MatrixLike OtherA, MatrixLike OtherB, MatrixLike... OtherRest>
        [[nodiscard]] APPKIT_MATRIX_OPT constexpr auto concat(const MatrixMul<OtherA, OtherB, OtherRest...> &other) const noexcept
        {
            return concat_impl(other, std::make_index_sequence<term_count>{},
                               std::make_index_sequence<MatrixMul<OtherA, OtherB, OtherRest...>::term_count>{});
        }

        template <MatrixLike, MatrixLike, MatrixLike...>
        friend class MatrixMul;

    private:
        using cache_matrix_t = std::conditional_t<(term_count > 2), Matrix<value_type, rows(), cols()>, detail::EmptyCache>;

        template <std::size_t L, std::size_t R>
        using segment_matrix_t = Matrix<value_type, term_t<L>::rows(), term_t<R>::cols()>;

        template <std::size_t L, std::size_t R>
        [[nodiscard]] APPKIT_MATRIX_OPT constexpr value_type eval_scalar_segment(std::size_t i, std::size_t j) const noexcept
        {
            if constexpr (L == R)
            {
                return std::get<L>(matrices_)(i, j);
            }
            else if constexpr ((L + 1) == R)
            {
                const auto &left = std::get<L>(matrices_);
                const auto &right = std::get<R>(matrices_);

                value_type sum = value_type{};

                if constexpr (requires(const term_t<L> &m) { m.row_data(0); } && requires(const term_t<R> &m) { m.row_data(0); })
                {
                    const value_type *left_row = left.row_data(i);
                    for (std::size_t k = 0; k < term_t<L>::cols(); ++k)
                        sum += left_row[k] * right.row_data(k)[j];
                }
                else if constexpr (requires(const term_t<L> &m) { m.row_data(0); })
                {
                    const value_type *left_row = left.row_data(i);
                    for (std::size_t k = 0; k < term_t<L>::cols(); ++k)
                        sum += left_row[k] * right(k, j);
                }
                else if constexpr (requires(const term_t<R> &m) { m.row_data(0); })
                {
                    for (std::size_t k = 0; k < term_t<L>::cols(); ++k)
                        sum += left(i, k) * right.row_data(k)[j];
                }
                else
                {
                    for (std::size_t k = 0; k < term_t<L>::cols(); ++k)
                        sum += left(i, k) * right(k, j);
                }

                return sum;
            }
            else
            {
                constexpr std::size_t split = split_plan_[L][R];

                value_type sum = value_type{};
                for (std::size_t k = 0; k < term_t<split>::cols(); ++k)
                    sum += eval_scalar_segment<L, split>(i, k) *
                           eval_scalar_segment<split + 1, R>(k, j);

                return sum;
            }
        }

        APPKIT_MATRIX_OPT constexpr void ensure_cache() const noexcept
        {
            if constexpr (term_count > 2)
            {
                if (!cache_valid_)
                {
                    eval_to(cache_);
                    cache_valid_ = true;
                }
            }
        }

        template <std::size_t L, std::size_t R, typename Fn>
        APPKIT_MATRIX_OPT constexpr void with_segment_operand(Fn &&fn) const
        {
            if constexpr (L == R)
            {
                std::forward<Fn>(fn)(std::get<L>(matrices_));
            }
            else
            {
                segment_matrix_t<L, R> segment{};
                eval_segment<L, R>(segment);
                std::forward<Fn>(fn)(segment);
            }
        }

        template <std::size_t L, std::size_t R, MatrixLike OutMat>
        APPKIT_MATRIX_OPT constexpr void eval_segment(OutMat &out) const noexcept
        {
            if constexpr (L == R)
            {
                std::get<L>(matrices_).eval_to(out);
            }
            else
            {
                constexpr std::size_t split = split_plan_[L][R];

                with_segment_operand<L, split>(
                    [&](const auto &left_operand)
                    { with_segment_operand<split + 1, R>(
                          [&](const auto &right_operand)
                          { detail::mul_kernel(left_operand, right_operand, out); }); });
            }
        }

        template <MatrixLike MatNext, std::size_t... Is>
        [[nodiscard]] APPKIT_MATRIX_OPT constexpr auto append_impl(const MatNext &next, std::index_sequence<Is...>) const noexcept
        {
            return MatrixMul<MatA, MatB, Rest..., MatNext>(std::get<Is>(matrices_)..., next);
        }

        template <MatrixLike MatPrev, std::size_t... Is>
        [[nodiscard]] APPKIT_MATRIX_OPT constexpr auto prepend_impl(const MatPrev &prev, std::index_sequence<Is...>) const noexcept
        {
            return MatrixMul<MatPrev, MatA, MatB, Rest...>(prev, std::get<Is>(matrices_)...);
        }

        template <MatrixLike OtherA, MatrixLike OtherB, MatrixLike... OtherRest, std::size_t... Is, std::size_t... Js>
        [[nodiscard]] APPKIT_MATRIX_OPT constexpr auto concat_impl(const MatrixMul<OtherA, OtherB, OtherRest...> &other,
                                                                   std::index_sequence<Is...>,
                                                                   std::index_sequence<Js...>) const noexcept
        {
            return MatrixMul<MatA, MatB, Rest..., OtherA, OtherB, OtherRest...>(std::get<Is>(matrices_)..., std::get<Js>(other.matrices_)...);
        }

        static constexpr auto dims_ = detail::chain_dims<mats_tuple>();
        static constexpr auto split_plan_ = []()
        {
            if constexpr (term_count >= dp_min_term_count)
                return detail::chain_split_plan<term_count>(dims_);
            else
                return detail::linear_split_plan<term_count>();
        }();

        [[no_unique_address]] mutable cache_matrix_t cache_{};
        mutable bool cache_valid_ = false;
        const std::tuple<const MatA &, const MatB &, const Rest &...> matrices_;
    };

    template <MatrixLike MatA, MatrixLike MatB, MatrixLike MatC>
    APPKIT_MATRIX_OPT constexpr void mul_to(const MatA &matrix_a, const MatB &matrix_b, MatC &result) noexcept
    {
        static_assert(MatA::cols() == MatB::rows(), "Inner matrix dimensions must match");
        static_assert(MatA::rows() == MatC::rows() && MatB::cols() == MatC::cols(), "Result matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type> &&
                          std::is_same_v<typename MatA::value_type, typename MatC::value_type>,
                      "Matrix value types must match");

        detail::mul_kernel(matrix_a, matrix_b, result);
    }

    template <MatrixLike MatA, MatrixLike MatB>
    APPKIT_MATRIX_OPT constexpr auto operator*(const MatA &matrix_a, const MatB &matrix_b) noexcept
    {
        return MatrixMul(matrix_a, matrix_b);
    }

    template <MatrixLike MatA, MatrixLike MatB, MatrixLike... Rest, MatrixLike MatNext>
    APPKIT_MATRIX_OPT constexpr auto operator*(const MatrixMul<MatA, MatB, Rest...> &matrix_a, const MatNext &matrix_b) noexcept
    {
        return matrix_a.append(matrix_b);
    }

    template <MatrixLike MatPrev, MatrixLike MatA, MatrixLike MatB, MatrixLike... Rest>
    APPKIT_MATRIX_OPT constexpr auto operator*(const MatPrev &matrix_a, const MatrixMul<MatA, MatB, Rest...> &matrix_b) noexcept
    {
        return matrix_b.prepend(matrix_a);
    }

    template <MatrixLike MatA, MatrixLike MatB, MatrixLike... RestA,
              MatrixLike MatC, MatrixLike MatD, MatrixLike... RestB>
    APPKIT_MATRIX_OPT constexpr auto operator*(const MatrixMul<MatA, MatB, RestA...> &matrix_a,
                                               const MatrixMul<MatC, MatD, RestB...> &matrix_b) noexcept
    {
        return matrix_a.concat(matrix_b);
    }

#else
    template <MatrixLike MatA, MatrixLike MatB, MatrixLike MatC>
    APPKIT_MATRIX_OPT constexpr void mul_to(const MatA &matrix_a, const MatB &matrix_b, MatC &result) noexcept
    {
        static_assert(MatA::cols() == MatB::rows(), "Inner matrix dimensions must match");
        static_assert(MatA::rows() == MatC::rows() && MatB::cols() == MatC::cols(), "Result matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type> &&
                          std::is_same_v<typename MatA::value_type, typename MatC::value_type>,
                      "Matrix value types must match");

        detail::mul_kernel(matrix_a, matrix_b, result);
    }

    template <MatrixLike MatA, MatrixLike MatB>
    APPKIT_MATRIX_OPT constexpr auto operator*(const MatA &matrix_a, const MatB &matrix_b) noexcept
    {
        static_assert(MatA::cols() == MatB::rows(), "Inner matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>, "Matrix value types must match");

        using ResultType = Matrix<typename MatA::value_type, MatA::rows(), MatB::cols()>;
        ResultType result;
        mul_to(matrix_a, matrix_b, result);
        return result;
    }
#endif // APPKIT_MATRIX_USE_PROXY

    template <MatrixLike MatA, MatrixLike MatB>
    APPKIT_MATRIX_OPT constexpr MatA &operator*=(MatA &matrix_a, const MatB &matrix_b) noexcept
    {
        static_assert(MatA::cols() == MatB::rows(), "Inner matrix dimensions must match");
        static_assert(std::is_same_v<typename MatA::value_type, typename MatB::value_type>, "Matrix value types must match");

        using TempType = Matrix<typename MatA::value_type, MatA::rows(), MatB::cols()>;
        TempType temp;
        mul_to(matrix_a, matrix_b, temp);
        matrix_a = temp;
        return matrix_a;
    }
} // namespace appkit::math
