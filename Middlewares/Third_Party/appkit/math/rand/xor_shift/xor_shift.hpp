#pragma once

#include <macros.hpp>

namespace appkit::math
{
    /**
     * @brief 快速随机数生成算法
     */
    class XorShift
    {
        uint32_t seed_{}; ///< 当前种子

    public:
        /**
         * @brief 构造函数
         * @param seed 种子
         */
        explicit XorShift(uint32_t seed = 42);

        /**
         * @brief 设置种子
         * @param seed 种子
         */
        FORCE_INLINE void SetSeed(uint32_t seed)
        {
            seed_ = seed;
        }

        /**
         * @brief 获取下一个随机数
         * @return 随机数
         */
        uint32_t Get();
    };
} // namespace appkit::math
