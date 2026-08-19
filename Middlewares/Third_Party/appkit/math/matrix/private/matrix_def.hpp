#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

#include <macros.hpp>

#define APPKIT_MATRIX_USE_PROXY 1

#ifndef APPKIT_MATRIX_FORCE_OPT_MARKERS
#define APPKIT_MATRIX_FORCE_OPT_MARKERS 1
#endif

#if APPKIT_MATRIX_FORCE_OPT_MARKERS == 1
#if defined(__GNUC__)
#define APPKIT_MATRIX_OPT [[gnu::optimize("O2")]] FORCE_INLINE
#else
#define APPKIT_MATRIX_OPT FORCE_INLINE
#endif
#else
#define APPKIT_MATRIX_OPT inline
#endif

namespace appkit::math
{
    /**
     * @brief 矩阵概念约束
     *
     * @tparam Mat
     */
    template <typename Mat>
    concept MatrixLike = requires(std::remove_cvref_t<Mat> m) {
        typename Mat::value_type;

        requires std::is_arithmetic_v<typename Mat::value_type>;

        { m.rows() } -> std::convertible_to<std::size_t>;
        { m.cols() } -> std::convertible_to<std::size_t>;

        requires(m.rows() > 0 && m.cols() > 0);

        // { m(0, 0) } -> std::convertible_to<typename Mat::value_type &>;
        { std::as_const(m)(0, 0) } -> std::convertible_to<typename Mat::value_type>;
    };
} // namespace appkit::math
