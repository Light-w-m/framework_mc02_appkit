#include <stm32_gpio.hpp>

#ifdef HAL_GPIO_MODULE_ENABLED

#include <logger.hpp>

namespace appkit::stm32
{
    STM32GPIO *STM32GPIO::map_[16]{};

    constexpr ExtiId GetExtiId(uint16_t pin)
    {
        APPKIT_RAISE_IF((pin & (~GPIO_PIN_MASK)) != 0, "Invalid GPIO pin for EXTI");

        uint8_t pin_num = __builtin_ctz(pin);
#if defined(STM32F0) || defined(STM32G0) || defined(STM32L0)
        if (pin_num <= 1)
        {
            return ExtiId::EXTI_0_1;
        }
        else if (pin_num <= 3)
        {
            return ExtiId::EXTI_2_3;
        }
        else
        {
            return ExtiId::EXTI_4_15;
        }

#else
        if (pin_num <= 4)
        {
            return static_cast<ExtiId>(pin_num);
        }
        else if (pin_num <= 9)
        {
            return ExtiId::EXTI_5_9;
        }
        else
        {
            return ExtiId::EXTI_10_15;
        }

#endif

        return ExtiId::EXTI_ID_ERROR;
    }

    constexpr uint32_t STM32GPIO::GetPinIndex(uint16_t pin)
    {
        APPKIT_RAISE_IF((pin & (~GPIO_PIN_MASK)) != 0, "Invalid GPIO pin");

        switch (pin)
        {
        case GPIO_PIN_0:
            return 0;
        case GPIO_PIN_1:
            return 1;
        case GPIO_PIN_2:
            return 2;
        case GPIO_PIN_3:
            return 3;
        case GPIO_PIN_4:
            return 4;
        case GPIO_PIN_5:
            return 5;
        case GPIO_PIN_6:
            return 6;
        case GPIO_PIN_7:
            return 7;
        case GPIO_PIN_8:
            return 8;
        case GPIO_PIN_9:
            return 9;
        case GPIO_PIN_10:
            return 10;
        case GPIO_PIN_11:
            return 11;
        case GPIO_PIN_12:
            return 12;
        case GPIO_PIN_13:
            return 13;
        case GPIO_PIN_14:
            return 14;
        case GPIO_PIN_15:
            return 15;
        default:
            APPKIT_RAISE("Invalid GPIO pin");
        }
    }

    STM32GPIO::STM32GPIO(const char *name, GPIO_TypeDef *handle, uint16_t pin, IRQn_Type irq)
        : handle_(handle), pin_(pin), irq_(irq)
    {
        APPKIT_RAISE_IF_NOT(nullptr != name, "GPIO name must not be null");

        if (irq != NonMaskableInt_IRQn)
        {
            auto ins_ptr = &map_[GetPinIndex(pin)];
            APPKIT_RAISE_IF_NOT(nullptr == *ins_ptr, "GPIO instance already exists for this pin");
            *ins_ptr = this;
        }

        // 创建设备文件
        APPKIT_RAISE_IF_NOT(Check(Block::RegisterDevice(name)), "Failed to register GPIO device");
    }

    bool STM32GPIO::Read() const
    {
        return HAL_GPIO_ReadPin(handle_, pin_) == GPIO_PIN_SET;
    }

    ErrorCode STM32GPIO::Write(bool value)
    {
        HAL_GPIO_WritePin(handle_, pin_, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
        return ErrorCode::OK;
    }

    ErrorCode STM32GPIO::SetInterrupt(bool enable)
    {
        if (irq_ == NonMaskableInt_IRQn)
        {
            return ErrorCode::NOT_SUPPORTED;
        }

        if (enable)
        {
            HAL_NVIC_EnableIRQ(irq_);
        }
        else
        {
            HAL_NVIC_DisableIRQ(irq_);
        }

        return ErrorCode::OK;
    }

    void STM32GPIO::EXTICallback(uint16_t pin)
    {
        if (auto gpio = map_[GetPinIndex(pin)];
            nullptr != gpio)
        {
            gpio->cb_list.ForEach<GPIO::Callback>(
                [](GPIO::Callback &cb)
                {
                    cb.Call(true);
                    return ErrorCode::OK;
                });
        }
    }
} // namespace appkit::stm32

extern "C"
{
    [[gnu::optimize("O2")]] void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
    {
        appkit::stm32::STM32GPIO::EXTICallback(GPIO_Pin);
    }
}

#endif
