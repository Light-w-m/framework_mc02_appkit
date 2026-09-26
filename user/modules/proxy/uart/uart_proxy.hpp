#pragma once

#include <proxy.hpp>
#include <uart.hpp>
#include <osal_semaphore.hpp>
#include <osal_thread.hpp>

class UARTProxy final
{
public:
    /**
     * @brief 构造 UART 字节流代理
     * @param uart_name UART 设备名称
     * @param proxy_rx_queue_size 代理接收队列大小
     * @param read_thread_stack_size 接收线程栈大小
     */
    explicit UARTProxy(const char *uart_name,
                       size_t proxy_rx_queue_size = 64,
                       uint32_t read_thread_stack_size = 512);

    /**
     * @brief 获取代理对象
     */
    FORCE_INLINE appkit::Proxy &GetProxy() noexcept
    {
        return proxy_;
    }

    /**
     * @brief 获取 UART 对象
     */
    FORCE_INLINE appkit::Uart &GetUart() noexcept
    {
        return uart_;
    }

private:
    static constexpr size_t READ_CHUNK_SIZE = 128;

    appkit::Proxy proxy_;
    appkit::Uart uart_;

    appkit::osal::Semaphore readSemaphore_{};
    appkit::ReadOperation readOperation_{
        readSemaphore_, appkit::Duration::From<appkit::millisecond>(20)};
    appkit::osal::Thread readThread_{};

    uint8_t readBuffer_[READ_CHUNK_SIZE]{};

    /**
     * @brief 接收线程函数
     */
    static void ReadThreadFunc(UARTProxy *self);

    /**
     * @brief 发送回调
     * @param in_isr 是否在中断中调用
     * @param self UARTProxy 对象指针
     * @param data 待发送数据
     */
    static void SendCallback(bool in_isr, UARTProxy *self, appkit::ConstRawData data);
};
