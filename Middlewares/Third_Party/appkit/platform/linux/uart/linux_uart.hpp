/**
 * @file linux_uart.h
 * @author zmf
 * @brief Linux UART 相关定义
 * @date 2026-01-02
 */
#pragma once

#include <uart.hpp>
#include <osal_thread.hpp>
#include <osal_semaphore.hpp>
#include <string>

namespace appkit
{
    /**
     * @brief Linux平台串口设备
     */
    class LinuxUart final : public Uart::Block
    {
    public:
        /**
         * @brief 构造函数
         * @param name 设备名称
         * @param device_path 设备路径
         * @param tx_queue_size 传输队列大小
         * @param buffer_size 传输缓冲区大小
         * @param thread_stack_size 线程栈大小
         */
        LinuxUart(const char *name, const std::string &device_path, size_t tx_queue_size = 8, size_t buffer_size = 512, size_t thread_stack_size = 65536);

        /**
         * @brief 析构函数
         */
        virtual ~LinuxUart() override;

        /**
         * @brief 设置串口配置
         * @param config 配置参数
         */
        virtual ErrorCode SetConfig(const Uart::Config &config) override;

    protected:
        /**
         * @brief 写入函数
         * @param port 写入端口
         * @return 错误码
         */
        static ErrorCode WriteFun(WritePort &port);

        /**
         * @brief 读取函数
         * @param port 读取端口
         * @return 错误码
         */
        static ErrorCode ReadFun(ReadPort &port);

        /**
         * @brief 读取线程函数
         */
        void ReadThreadFunc();

        /**
         * @brief 写入线程函数
         */
        void WriteThreadFunc();

    private:
        const std::string devicePath_; ///< 设备路径
        int fd_{-1};                   ///< 文件描述符
        Uart::Config config_{};        ///< 当前配置
        bool connected_{false};        ///< 连接状态

        ReadPort read_;   ///< 读取端口
        WritePort write_; ///< 写入端口

        size_t buffer_size_{0}; ///< 传输缓冲区大小
        uint8_t *tx_buffer_;    ///< 传输缓冲区
        uint8_t *rx_buffer_;    ///< 接收缓冲区

        osal::Thread readThread_{};        ///< 读取线程
        osal::Thread writeThread_{};       ///< 写入线程
        osal::Semaphore writeSemaphore_{}; ///< 写入信号量
    };
}