#include <uart_proxy.hpp>

#include <algorithm>

UARTProxy::UARTProxy(const char *uart_name,
                     const size_t proxy_rx_queue_size,
                     const uint32_t read_thread_stack_size)
    : proxy_(proxy_rx_queue_size, appkit::Proxy::SendCallback::Create(SendCallback, this))
{
    APPKIT_RAISE_IF_NOT(appkit::Check(uart_.Open(uart_name)),
                        "Failed to open UART device");

    const auto *read_port = uart_.GetReadPort();
    APPKIT_RAISE_IF_NOT(read_port != nullptr && read_port->Readable() && read_port->buffer_ != nullptr,
                        "UART read port is not readable");

    const auto *write_port = uart_.GetWritePort();
    APPKIT_RAISE_IF_NOT(write_port != nullptr && write_port->Writable(),
                        "UART write port is not writable");

    APPKIT_RAISE_IF_NOT(
        appkit::Check(readThread_.Create(ReadThreadFunc, this, "uart_proxy_read",
                                         read_thread_stack_size,
                                         appkit::osal::Thread::Priority::HIGH)),
        "Failed to create UART proxy read thread");
}

void UARTProxy::ReadThreadFunc(UARTProxy *self)
{
    auto &read_port = *self->uart_.GetReadPort();

    for (;;)
    {
        // 零长度读取只用于等待 UART DMA 接收中断通知。
        const auto wait_ret = read_port({nullptr, 0}, self->readOperation_);
        if (!appkit::Check(wait_ret))
        {
            continue;
        }

        // ReadPort 的环形缓冲区可能一次收到多个代理帧，全部转交给 Proxy 解析。
        for (;;)
        {
            const auto available_size = read_port.buffer_->Size();
            if (available_size == 0)
            {
                break;
            }

            const auto chunk_size = std::min(available_size, sizeof(self->readBuffer_));
            if (!appkit::Check(read_port.buffer_->PopBatch(self->readBuffer_, chunk_size)))
            {
                break;
            }

            const auto ret = self->proxy_.PushData(
                false, appkit::ConstRawData{self->readBuffer_, chunk_size});
            if (!appkit::Check(ret))
            {
                APPKIT_LOG_WARNING("Failed to push UART data to proxy");
            }
        }
    }
}

void UARTProxy::SendCallback(bool in_isr, UARTProxy *self, appkit::ConstRawData data)
{
    // Uart::Write 会先把数据复制到驱动发送队列，因而回调返回后 data 仍可安全失效。
    appkit::WriteOperation operation;
    const auto ret = self->uart_.Write(data, operation);
    if (!appkit::Check(ret) && !in_isr)
    {
        APPKIT_LOG_ERROR("Failed to write UART proxy data");
    }
}



