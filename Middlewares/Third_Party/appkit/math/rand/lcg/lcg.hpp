#pragma once

#include <macros.hpp>

namespace appkit::math
{
    /**
     * @brief 线性同余算法
     * @note x_{n+1} = (a * x_{n} + b) mod m
     */
    class LCG
    {
        const uint64_t a_, b_, m_; ///< 乘数、增量、模数
        uint64_t seed_;            ///< 当前种子

    public:
        /**
         * @brief 构造函数
         * @param seed 种子
         * @param a 乘数
         * @param b 增量
         * @param m 模数
         */
        explicit LCG(uint64_t seed = 42, uint64_t a = 25214903917, uint64_t b = 11, uint64_t m = (1llu << 48) - 1);

        /**
         * @brief 设置种子
         * @param seed 种子
         */
        FORCE_INLINE void SetSeed(uint64_t seed)
        {
            seed_ = seed;
        }

        /**
         * @brief 获取下一个随机数
         * @return 随机数
         */
        uint64_t Get();
    };
} // namespace appkit::math
