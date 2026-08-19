#pragma once

#include <spi.hpp>

namespace led
{
    /**
     * @brief WS2812 LED 驱动类
     * @note 使用SPI模拟WS2812B协议，SPI具体参数如下：
     * - 波特率：6.0MBits/s
     * - 数据位：8位
     */
    class WS2812
    {
        static constexpr inline uint8_t LOW = 0xC0;  ///< 0码
        static constexpr inline uint8_t HIGH = 0xF0; ///< 1码

        appkit::SPI handle_{};                                                   ///< SPI设备
        appkit::WriteOperation op_{appkit::WriteOperation::PollingStatus::DONE}; ///< 写操作

        uint8_t txbuf[32];                   ///< 发送缓冲区
        appkit::ConstRawData txdata_{txbuf}; ///< 发送数据

    public:
        /**
         * @brief 构造函数
         * @param spi_name SPI设备名称
         */
        WS2812(const char *spi_name);

        /**
         * @brief 析构函数
         */
        ~WS2812();

        /**
         * @brief 设置颜色
         * @param r 红色分量，0-255
         * @param g 绿色分量，0-255
         * @param b 蓝色分量，0-255
         * @return 错误码
         * @note 该函数会将颜色转换为WS2812B的GRB格式，并通过SPI发送
         * @note 该函数是非阻塞的，如果上一次设置颜色还未完成，则返回ErrorCode::Busy
         */
        appkit::ErrorCode SetColor(uint8_t r, uint8_t g, uint8_t b);
    };
}