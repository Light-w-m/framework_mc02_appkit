#pragma once

/**
 * @file linux_uart.h
 * @author zmf
 * @brief Linux UART 相关定义
 * @date 2026-01-02
 */
#pragma once

#include <random.hpp>
#include <osal_mutex.hpp>
#include <string>

namespace appkit
{
    /**
     * @brief Linux平台串口设备
     */
    class LinuxRandom final : public Random::Block
    {
    public:
        /**
         * @brief 构造函数
         * @param name 设备名称
         */
        LinuxRandom(const char *name);

        /**
         * @brief 析构函数
         */
        virtual ~LinuxRandom() override;

        /**
         * @brief 获取原始随机数
         * @return 原始随机数
         */
        virtual size_t GetRawNum() override;

    private:
        int fd_{-1};                   ///< 文件描述符
        osal::Mutex mutex_{};          ///< 互斥锁
    };
}