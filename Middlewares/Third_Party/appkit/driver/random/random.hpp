#pragma once

#include <driver.hpp>

namespace appkit
{
    /**
     * @brief 随机数生成器
     */
    class Random final
    {
    public:
        /**
         * @brief 随机数生成器块基类
         */
        class Block : public Driver<Block>
        {
        public:
            /**
             * @brief 析构函数
             */
            virtual ~Block() = default;

            /**
             * @brief 获取原始随机数
             * @return 原始随机数
             */
            virtual size_t GetRawNum() = 0;
        };

    protected:
        Block *block_{};      ///< 设备块

    public:
        /**
         * @brief 构造函数
         */
        constexpr Random() = default;

        /**
         * @brief 析构函数
         */
        ~Random() = default;

        /**
         * @brief 打开随机数生成器设备
         * @param name 设备名称
         * @return 错误码
         */
        ErrorCode Open(const char *name);

        /**
         * @brief 获取指定范围的随机数
         * @param min 最小值，默认为0
         * @param max 最大值，默认为SIZE_MAX
         * @return 随机数
         */
        [[nodiscard]] size_t GetRandom(size_t min = 0, size_t max = SIZE_MAX) const;

        /**
         * @brief 获取指定范围的浮点数随机数
         * @param min 最小值
         * @param max 最大值
         * @return 随机浮点数
         */
        [[nodiscard]] float GetRandom(float min, float max) const;
    };
}
