#pragma once

#if __has_include(<main.h>)
#include <main.h>
#endif

#ifdef HAL_UART_MODULE_ENABLED

#include <usart.h>
#include <uart.hpp>

#include <double_buffer.hpp>

namespace appkit::stm32
{
    enum class UartId
    {
#ifdef USART1
        STM32_USART1,
#endif
#ifdef USART2
        STM32_USART2,
#endif
#ifdef USART3
        STM32_USART3,
#endif
#ifdef USART4
        STM32_USART4,
#endif
#ifdef USART5
        STM32_USART5,
#endif
#ifdef USART6
        STM32_USART6,
#endif
#ifdef USART7
        STM32_USART7,
#endif
#ifdef USART8
        STM32_USART8,
#endif
#ifdef USART9
        STM32_USART9,
#endif
#ifdef USART10
        STM32_USART10,
#endif
#ifdef USART11
        STM32_USART11,
#endif
#ifdef USART12
        STM32_USART12,
#endif
#ifdef USART13
        STM32_USART13,
#endif
#ifdef UART1
        STM32_UART1,
#endif
#ifdef UART2
        STM32_UART2,
#endif
#ifdef UART3
        STM32_UART3,
#endif
#ifdef UART4
        STM32_UART4,
#endif
#ifdef UART5
        STM32_UART5,
#endif
#ifdef UART6
        STM32_UART6,
#endif
#ifdef UART7
        STM32_UART7,
#endif
#ifdef UART8
        STM32_UART8,
#endif
#ifdef UART9
        STM32_UART9,
#endif
#ifdef UART10
        STM32_UART10,
#endif
#ifdef UART11
        STM32_UART11,
#endif
#ifdef UART12
        STM32_UART12,
#endif
#ifdef UART13
        STM32_UART13,
#endif
#ifdef LPUART1
        STM32_LPUART1,
#endif
#ifdef LPUART2
        STM32_LPUART2,
#endif
#ifdef LPUART3
        STM32_LPUART3,
#endif
        STM32_UART_NUMBER,
        STM32_UART_ID_ERROR
    };

    constexpr UartId GetUartId(const UART_HandleTypeDef *handle);

    class STM32Uart final : public Uart::Block
    {
        static inline STM32Uart *map_[std::to_underlying(UartId::STM32_UART_NUMBER)]{};

        UART_HandleTypeDef *handle_{};
        UartId id_{UartId::STM32_UART_ID_ERROR};

        ReadPort read_{};
        WritePort write_{};

        DoubleBuffer write_buffer_;
        RawData read_buffer_;

        WriteInfo current_info_{};

        size_t last_rx_pos_{};

    protected:
        static ErrorCode WriteFun(WritePort &port);

        static ErrorCode ReadFun(ReadPort &port);

    public:
        STM32Uart(const char *name, UART_HandleTypeDef &handle, RawData tx_buff,
                  RawData rx_buff, uint32_t tx_queue_size = 5);

        static void RxCpltHandler(UartId id);

        static void TxCpltHandler(UartId id);

        static void ErrorHandler(UartId id);
    };
}

#endif // HAL_UART_MODULE_ENABLED
