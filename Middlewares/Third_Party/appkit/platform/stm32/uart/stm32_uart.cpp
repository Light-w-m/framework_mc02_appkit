#include "stm32_uart.hpp"

#ifdef HAL_UART_MODULE_ENABLED

#include <logger.hpp>

namespace appkit::stm32
{
    constexpr UartId GetUartId(const UART_HandleTypeDef *handle)
    {
        const auto addr = handle->Instance;
        if (addr == nullptr)
            return UartId::STM32_UART_ID_ERROR;
#ifdef USART1
        if (addr == USART1)
            return UartId::STM32_USART1;
#endif
#ifdef USART2
        if (addr == USART2)
            return UartId::STM32_USART2;
#endif
#ifdef USART3
        if (addr == USART3)
            return UartId::STM32_USART3;
#endif
#ifdef USART4
        if (addr == USART4)
            return UartId::STM32_USART4;
#endif
#ifdef USART5
        if (addr == USART5)
            return UartId::STM32_USART5;
#endif
#ifdef USART6
        if (addr == USART6)
            return UartId::STM32_USART6;
#endif
#ifdef USART7
        if (addr == USART7)
            return UartId::STM32_USART7;
#endif
#ifdef USART8
        if (addr == USART8)
            return UartId::STM32_USART8;
#endif
#ifdef USART9
        if (addr == USART9)
            return UartId::STM32_USART9;
#endif
#ifdef USART10
        if (addr == USART10)
            return UartId::STM32_USART10;
#endif
#ifdef USART11
        if (addr == USART11)
            return UartId::STM32_USART11;
#endif
#ifdef USART12
        if (addr == USART12)
            return UartId::STM32_USART12;
#endif
#ifdef USART13
        if (addr == USART13)
            return UartId::STM32_USART13;
#endif
#ifdef UART1
        if (addr == UART1)
            return UartId::STM32_UART1;
#endif
#ifdef UART2
        if (addr == UART2)
            return UartId::STM32_UART2;
#endif
#ifdef UART3
        if (addr == UART3)
            return UartId::STM32_UART3;
#endif
#ifdef UART4
        if (addr == UART4)
            return UartId::STM32_UART4;
#endif
#ifdef UART5
        if (addr == UART5)
            return UartId::STM32_UART5;
#endif
#ifdef UART6
        if (addr == UART6)
            return UartId::STM32_UART6;
#endif
#ifdef UART7
        if (addr == UART7)
            return UartId::STM32_UART7;
#endif
#ifdef UART8
        if (addr == UART8)
            return UartId::STM32_UART8;
#endif
#ifdef UART9
        if (addr == UART9)
            return UartId::STM32_UART9;
#endif
#ifdef UART10
        if (addr == UART10)
            return UartId::STM32_UART10;
#endif
#ifdef UART11
        if (addr == UART11)
            return UartId::STM32_UART11;
#endif
#ifdef UART12
        if (addr == UART12)
            return UartId::STM32_UART12;
#endif
#ifdef UART13
        if (addr == UART13)
            return UartId::STM32_UART13;
#endif
#ifdef LPUART1
        if (addr == LPUART1)
            return UartId::STM32_LPUART1;
#endif
#ifdef LPUART2
        if (addr == LPUART2)
            return UartId::STM32_LPUART2;
#endif
#ifdef LPUART3
        if (addr == LPUART3)
            return UartId::STM32_LPUART3;
#endif

        return UartId::STM32_UART_ID_ERROR;
    }

    STM32Uart::STM32Uart(const char *name, UART_HandleTypeDef &handle, RawData tx_buff, const RawData rx_buff,
                         uint32_t tx_queue_size)
        : handle_(&handle), id_{GetUartId(&handle)}, read_{rx_buff.GetSize()}, write_{tx_queue_size, tx_queue_size * tx_buff.GetSize() / 2},
          write_buffer_{tx_buff}, read_buffer_{rx_buff}
    {
        // 检查是否重复创建
        APPKIT_RAISE_IF_NOT(nullptr == RamFs::Dev().Find(name), "Device already exists");

        // 初始化输入输出端口
        read = &read_;
        write = &write_;

        // 注册到映射表
        map_[std::to_underlying(id_)] = this;

        // 初始化UART外设
        // 判断是否使能发送
        if ((handle_->Init.Mode & UART_MODE_TX) == UART_MODE_TX)
        {
            // 判断是否支持DMA发送
            if (nullptr == handle_->hdmatx)
            {
                APPKIT_LOG_WARNING("Device <%s> doesn't support DMA send", name);
                APPKIT_RAISE("Device doesn't support DMA send");
            }

            // 开启接收完成中断
            __HAL_UART_ENABLE_IT(handle_, UART_IT_TC);

            write_ = WriteFun;
        }

        // 判断是否使能接收
        if ((handle_->Init.Mode & UART_MODE_RX) == UART_MODE_RX)
        {
            // 判断是否支持DMA接收
            if (nullptr == handle_->hdmarx)
            {
                APPKIT_LOG_WARNING("Device <%s> doesn't support DMA recv", name);
                APPKIT_RAISE("Device doesn't support DMA recv");
            }
            else
            {
                // 配置DMA为循环搬运模式
                handle_->hdmarx->Init.Mode = DMA_CIRCULAR;
                APPKIT_RAISE_IF_NOT(HAL_OK == HAL_DMA_Init(handle_->hdmarx), "Failed to init UART RX DMA");

                APPKIT_RAISE_IF_NOT(HAL_OK ==
                                        HAL_UARTEx_ReceiveToIdle_DMA(handle_, read_buffer_.GetData<uint8_t>(), read_buffer_.GetSize()),
                                    "Failed to start UART RX DMA");
            }

            // 开启接收完成中断
            __HAL_UART_ENABLE_IT(handle_, UART_IT_IDLE);

            read_ = ReadFun;
        }

        // 注册设备文件
        APPKIT_RAISE_IF_NOT(Check(Block::RegisterDevice(name)), "Failed to register device");
    }

    ErrorCode STM32Uart::ReadFun(ReadPort &port)
    {
        const auto uart = CONTAINER_OF(&port, STM32Uart, read_);
        UNUSED(uart);

        return ErrorCode::EMPTY;
    }

    ErrorCode STM32Uart::WriteFun(WritePort &port)
    {
        if (const auto uart = CONTAINER_OF(&port, STM32Uart, write_);
            !uart->write_buffer_.IsPendingValid())
        {
            // 读取写入端口的调用信息
            WriteInfo info;
            if (!Check(port.info_buffer_->Peek(info)))
            {
                return ErrorCode::EMPTY;
            }

            const auto to_write = std::min(info.data.GetSize(), uart->write_buffer_.GetCapacity());

            bool use_pending{};
            uint8_t *buffer{};
            if ((HAL_UART_GetState(uart->handle_) & HAL_UART_STATE_BUSY_TX) == HAL_UART_STATE_READY)
            {
                // 上一次发送已完成，直接使用主缓冲区
                buffer = uart->write_buffer_.GetActiveBuffer().GetData<uint8_t>();
            }
            else
            {
                // 上一次发送已未完成，写入副缓冲区
                buffer = uart->write_buffer_.GetPendingBuffer().GetData<uint8_t>();
                use_pending = true;
            }

            if (const auto ret = port.data_buffer_->PopBatch(buffer, to_write);
                ret != ErrorCode::OK)
            {
                return ret;
            }

            if (use_pending)
            {
                uart->write_buffer_.SetPendingUsed(to_write);

                // 再次检查串口是否空闲
                if ((HAL_UART_GetState(uart->handle_) & HAL_UART_STATE_BUSY_TX) == HAL_UART_STATE_READY && uart->write_buffer_.IsPendingValid())
                {
                    uart->write_buffer_.Switch();
                }
                else
                {
                    // 仍然使用副缓冲区发送
                    return ErrorCode::BUSY;
                }
            }

            port.info_buffer_->Pop(uart->current_info_);

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
            SCB_CleanDCache_by_Addr(uart->write_buffer_.GetActiveBuffer().GetData<uint32_t>(), to_write);
#endif

            auto ret = HAL_UART_Transmit_DMA(
                uart->handle_, uart->write_buffer_.GetActiveBuffer().GetData<uint8_t>(), info.data.GetSize());

            if (HAL_OK != ret)
            {
                port.Finish(false, ErrorCode::FAILED, info, 0);
                return ErrorCode::FAILED;
            }

            return ErrorCode::OK;
        }

        return ErrorCode::FAILED;
    }

    __RAM_FUNC void STM32Uart::RxCpltHandler(UartId id)
    {
        const auto uart = map_[std::to_underlying(id)];
        uint8_t *rx_buffer = uart->read_buffer_.GetData<uint8_t>();

        size_t size = uart->read_buffer_.GetSize();
        const size_t current_pos = size - uart->handle_->RxXferCount;
        const size_t last_pos = uart->last_rx_pos_;

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
        SCB_InvalidateDCache_by_Addr(static_cast<void *>(rx_buffer), size);
#endif

        // 使用DMA接收
        // 判断是否正确接收到数据
        if (current_pos == last_pos)
            return;

        if (current_pos > last_pos)
        {
            // 接收区线性
            uart->read_.buffer_->PushBatch(&rx_buffer[last_pos], current_pos - last_pos);
        }
        else
        {
            uart->read_.buffer_->PushBatch(&rx_buffer[last_pos], size - last_pos);
            uart->read_.buffer_->PushBatch(&rx_buffer[0], current_pos);
        }

        uart->last_rx_pos_ = current_pos;

        uart->read_.ProcessPendingReads(true);
    }

    __RAM_FUNC void STM32Uart::TxCpltHandler(UartId id)
    {
        const auto uart = map_[std::to_underlying(id)];

        auto &info = uart->current_info_;

        // 完成上一次传输
        uart->write_.Finish(true, (HAL_UART_GetError(uart->handle_) == HAL_OK) ? ErrorCode::OK : ErrorCode::FAILED, info, info.data.GetSize());

        // 获取待发送数据长度
        const size_t pending_length{uart->write_buffer_.GetPendingLength()};
        if (uart->write_buffer_.IsPendingValid() == false)
        {
            // 没有待发送的数据
            return;
        }

        // 切换缓冲区
        uart->write_buffer_.Switch();

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
        SCB_CleanDCache_by_Addr(uart->write_buffer_.GetActiveBuffer().GetData<uint32_t>(),
                                pending_length);
#endif

        auto ret = HAL_UART_Transmit_DMA(
            uart->handle_, uart->write_buffer_.GetActiveBuffer().GetData<uint8_t>(), pending_length);

        if (HAL_OK != ret)
        {
            APPKIT_RAISE_FROM_CALLBACK(true, "Failed to start pending UART DMA transmit");
        }

        // 实际弹出调用方信息
        if (!Check(uart->write_.info_buffer_->Pop(info)))
        {
            APPKIT_RAISE_FROM_CALLBACK(true, "UART write info buffer underflow");
        }

        // 检测发送队列中是否还存在未发送数据
        WriteInfo next_info;
        if (!Check(uart->write_.info_buffer_->Peek(next_info)))
        {
            // 发送队列已清空
            return;
        }

        // 写入副缓冲区
        const auto to_write = std::min(next_info.data.GetSize(), uart->write_buffer_.GetCapacity());
        if (Check(uart->write_.data_buffer_->PopBatch(uart->write_buffer_.GetPendingBuffer().GetData<uint8_t>(), to_write)))
        {
            uart->write_buffer_.SetPendingUsed(to_write);
        }
    }

    __RAM_FUNC void STM32Uart::ErrorHandler(UartId id)
    {
        const auto uart = map_[std::to_underlying(id)];
        const auto error = HAL_UART_GetError(uart->handle_);

        UNUSED(error);

        // 重启接收
        uart->last_rx_pos_ = 0;
        HAL_UARTEx_ReceiveToIdle_DMA(uart->handle_, uart->read_buffer_.GetData<uint8_t>(),
                                     uart->read_buffer_.GetSize());

        // 重启发送
        HAL_UART_AbortTransmit(uart->handle_);
        uart->TxCpltHandler(id);
    }
}

extern "C"
{
    using namespace appkit::stm32;

    [[gnu::optimize("O2")]] void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
    {
        STM32Uart::TxCpltHandler(GetUartId(huart));
    }

    [[gnu::optimize("O2")]] void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
    {
        UNUSED(Size);
        STM32Uart::RxCpltHandler(GetUartId(huart));
    }

    [[gnu::optimize("O2")]] void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
    {
        STM32Uart::ErrorHandler(GetUartId(huart));
    }
}

#endif // HAL_UART_MODULE_ENABLED
