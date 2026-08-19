#pragma once

#if __has_include(<main.h>)
#include <main.h>
#endif

#ifdef HAL_GPIO_MODULE_ENABLED

#include <gpio.h>
#include <gpio.hpp>

namespace appkit::stm32
{
    enum class ExtiId : uint8_t
    {
#if defined(STM32F0) || defined(STM32G0) || defined(STM32L0)
        EXTI_0_1,
        EXTI_2_3,
        EXTI_4_15,
#elif defined(STM32WB0)
        EXTI_GPIOA,
        EXTI_GPIOB,
#else
        EXTI_0,
        EXTI_1,
        EXTI_2,
        EXTI_3,
        EXTI_4,
        EXTI_5_9,
        EXTI_10_15,
#endif
        EXTI_NUMBER,
        EXTI_ID_ERROR
    };

    constexpr ExtiId GetExtiId(uint16_t pin);

    class STM32GPIO final : public GPIO::Block
    {
        static STM32GPIO* map_[16];

        GPIO_TypeDef *handle_;
        uint16_t pin_;
        IRQn_Type irq_;

        static constexpr uint32_t GetPinIndex(uint16_t pin);

    public:
        /**
         * @brief 构造函数
         * @param name 设备名称
         * @param handle GPIO端口句柄
         * @param pin GPIO引脚号
         * @param irq 中断号，默认不使用中断
         */
        STM32GPIO(const char *name, GPIO_TypeDef *handle, uint16_t pin, IRQn_Type irq = NonMaskableInt_IRQn);

        bool Read() const override;

        ErrorCode Write(bool value) override;

        ErrorCode SetInterrupt(bool enable) override;

        static void EXTICallback(uint16_t pin);
    };
}

#endif
