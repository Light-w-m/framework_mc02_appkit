#pragma once

#include <common_type.hpp>

namespace appkit::math
{
    /**
     * @brief CRC16计算类
     */
    class Crc16 final
    {
        static const uint16_t table_[256]; ///< CRC16查找表
        static const uint16_t start_;      ///< 初始CRC16值

    public:
        /**
         * @brief 计算CRC16值
         * @param data 输入数据
         * @return CRC16值
         */
        static uint16_t Calculate(ConstRawData data);

        /**
         * @brief 校验CRC16值
         * @param data 输入数据
         * @param crc16 期望的CRC16值
         * @return 校验结果，true表示校验通过，false表示校验失败
         */
        FORCE_INLINE static bool Checksum(ConstRawData data, uint16_t crc16)
        {
            return Calculate(data) == crc16;
        }
    };
} // namespace LIB_NAMESPACE
