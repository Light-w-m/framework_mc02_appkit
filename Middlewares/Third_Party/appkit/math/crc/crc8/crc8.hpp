#pragma once

#include <common_type.hpp>

namespace appkit::math
{
    /**
     * @brief CRC8计算类
     */
    class Crc8 final
    {
        static const uint8_t table_[256]; ///< CRC8查找表
        static const uint8_t start_;      ///< 初始CRC8值

    public:
        /**
         * @brief 计算CRC8值
         * @param data 输入数据
         * @return CRC8值
         */
        static uint8_t Calculate(ConstRawData data);

        /**
         * @brief 校验CRC8值
         * @param data 输入数据
         * @param crc8 期望的CRC8值
         * @return 校验结果，true表示校验通过，false表示校验失败
         */
        FORCE_INLINE static bool Checksum(ConstRawData data, uint8_t crc8)
        {
            return Calculate(data) == crc8;
        }
    };
} // namespace LIB_NAMESPACE
