#pragma once

#include <driver.hpp>

namespace appkit
{
    /**
     * @brief ADC设备
     */
    class ADC final
    {
    public:
        /**
         * @brief ADC设备块基类
         */
        class Block : public Driver<Block>
        {
        public:
            virtual ~Block() = default;
            virtual Result<float> Read() = 0;
        };

        /**
         * @brief 构造函数
         */
        constexpr ADC() = default;

        /**
         * @brief 打开ADC设备
         * @param name 设备名称
         * @return 错误码
         */
        ErrorCode Open(const char *name);

        /**
         * @brief 读取ADC值
         * @param value 存放读取值的引用
         * @return 错误码
         */
        [[nodiscard]] Result<float> Read() const;

    private:
        Block *block_{}; ///< 设备块指针
    };
}
