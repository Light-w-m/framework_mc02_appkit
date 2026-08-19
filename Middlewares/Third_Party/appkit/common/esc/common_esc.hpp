#pragma once

#include <macros.hpp>

#ifndef CSI
#define CSI(code) "\033[" #code "m"
#else
#error "CSI macro redefined"
#endif

namespace appkit
{
    class Format final
    {
    private:
        static constexpr inline const char *const format_str_[7]{
            "", CSI(0), CSI(1), CSI(2),
            CSI(4), CSI(5), CSI(7)}; ///< 格式字符串

    public:
        /**
         * @brief 格式类型
         */
        enum class Type : uint8_t
        {
            NONE = 0,  ///< 无格式
            NORMAL,    ///< 正常
            BOLD,      ///< 粗体
            DARK,      ///< 暗色
            UNDERLINE, ///< 下划线
            BLINK,     ///< 闪烁
            REVERSE,   ///< 反显
        };

        /**
         * @brief 获取格式字符串
         * @param type 格式类型
         * @return 格式字符串
         */
        FORCE_INLINE static constexpr auto GetStr(Type type)
        {
            return format_str_[std::to_underlying(type)];
        }
    };

    class Color final
    {
    private:
        static constexpr inline const char *const color_str_[9]{
            "", CSI(30), CSI(31), CSI(32), CSI(33),
            CSI(34), CSI(35), CSI(36), CSI(37)}; ///< 颜色字符串

    public:
        /**
         * @brief 颜色类型
         */
        enum class Type : uint8_t
        {
            NONE = 0, ///< 无
            BLACK,    ///< 黑色
            RED,      ///< 红色
            GREEN,    ///< 绿色
            YELLOW,   ///< 黄色
            BLUE,     ///< 蓝色
            MAGENTA,  ///< 品红色
            CYAN,     ///< 青色
            WHITE,    ///< 白色
        };

        /**
         * @brief 获取前景色字符串
         * @param type 颜色类型
         * @return 颜色字符串
         */
        FORCE_INLINE static constexpr auto GetStr(Type type)
        {
            return color_str_[std::to_underlying(type)];
        }
    };

    class Background final
    {
    private:
        static constexpr inline const char *const background_str_[9]{
            "", CSI(40), CSI(41), CSI(42), CSI(43),
            CSI(44), CSI(45), CSI(46), CSI(47)}; ///< 背景色字符串

    public:
        /**
         * @brief 背景色类型
         */
        enum class Type : uint8_t
        {
            NONE = 0, ///< 无
            BLACK,    ///< 黑色
            RED,      ///< 红色
            GREEN,    ///< 绿色
            YELLOW,   ///< 黄色
            BLUE,     ///< 蓝色
            MAGENTA,  ///< 品红色
            CYAN,     ///< 青色
            WHITE,    ///< 白色
        };

        /**
         * @brief 获取背景色字符串
         * @param type 背景色类型
         * @return 背景色字符串
         */
        FORCE_INLINE static constexpr auto GetStr(Type type)
        {
            return background_str_[std::to_underlying(type)];
        }
    };
}

#undef CSI
