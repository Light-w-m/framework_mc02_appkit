#pragma once

#include <macros.hpp>

namespace appkit
{
    namespace math::fast
    {
        /**
         * @brief 快速倒数平方根函数
         * @param x 输入值
         * @return x的倒数平方根的近似值
         */
        [[using gnu: optimize("O3")]] inline constexpr float InvSqrt(float x) noexcept
        {
            if (x <= 0.0f)
            {
                return 0.0f; // 对于非正数，返回0以避免错误
            }

            uint32_t i = std::bit_cast<uint32_t>(x); // 将浮点数的二进制表示作为整数处理
            i = 0x5f3759df - (i >> 1);               // 初始近似值

            float y = std::bit_cast<float>(i);     // 将整数重新解释为浮点数
            y = y * (1.5f - (x * 0.5f * (y * y))); // 牛顿迭代
            y = y * (1.5f - (x * 0.5f * (y * y))); // 牛顿迭代

            return y; // 返回倒数平方根的近似值
        }

        /**
         * @brief 快速平方根函数
         * @param x 输入值
         * @return x的平方根的近似值
         */
        [[using gnu: optimize("O3")]] inline constexpr float Sqrt(float x) noexcept
        {
            return x * InvSqrt(x); // 使用倒数平方根计算平方根
        }
    }

    /**
     * @brief 数学相关的用户自定义字面值后缀
     */
    namespace math_literals
    {
        /**
         * @brief 数字后缀：十倍
         * @param value 数值
         * @return 计算结果
         */
        FORCE_INLINE consteval unsigned long long operator""_hundrads(unsigned long long value) noexcept
        {
            return value * 100ull;
        }

        /**
         * @brief 数字后缀：千倍
         * @param value 数值
         * @return 计算结果
         */
        FORCE_INLINE consteval unsigned long long operator""_thousands(unsigned long long value) noexcept
        {
            return value * 1000ull;
        }

        /**
         * @brief 数字后缀：百万倍
         * @param value 数值
         * @return 计算结果
         */
        FORCE_INLINE consteval unsigned long long operator""_millions(unsigned long long value) noexcept
        {
            return value * 1000_thousands;
        }

        /**
         * @brief 数字后缀：十亿倍
         * @param value 数值
         * @return 计算结果
         */
        FORCE_INLINE consteval unsigned long long operator""_billions(unsigned long long value) noexcept
        {
            return value * 1000_millions;
        }
    } // namespace math_literals

    /**
     * @brief 存储相关的用户自定义字面值后缀
     */
    namespace storage_literals
    {
        /**
         * @brief 存储单位后缀：千字节
         * @param value 数值
         * @return 计算结果
         */
        FORCE_INLINE consteval unsigned long long operator""_kilo(unsigned long long value) noexcept
        {
            return value * 1024ull;
        }

        /**
         * @brief 存储单位后缀：兆字节
         * @param value 数值
         * @return 计算结果
         */
        FORCE_INLINE consteval unsigned long long operator""_mega(unsigned long long value) noexcept
        {
            return value * 1024_kilo;
        }

        /**
         * @brief 存储单位后缀：吉字节
         * @param value 数值
         * @return 计算结果
         */
        FORCE_INLINE consteval unsigned long long operator""_giga(unsigned long long value) noexcept
        {
            return value * 1024_mega;
        }
    } // namespace storage_literals

} // namespace appkit