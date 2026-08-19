#include <stm32_iwdg.hpp>

#ifdef HAL_IWDG_MODULE_ENABLED

#include <logger.hpp>

namespace appkit::stm32
{
    FORCE_INLINE static uint16_t GetPrescaler(IWDG_HandleTypeDef *handle)
    {
        // 获取IWDG预分频值
        switch (const uint32_t prescaler = handle->Init.Prescaler; prescaler)
        {
        case IWDG_PRESCALER_4:
            return 4;
        case IWDG_PRESCALER_8:
            return 8;
        case IWDG_PRESCALER_16:
            return 16;
        case IWDG_PRESCALER_32:
            return 32;
        case IWDG_PRESCALER_64:
            return 64;
        case IWDG_PRESCALER_128:
            return 128;
        case IWDG_PRESCALER_256:
            return 256;
        default:
            APPKIT_RAISE("Invalid IWDG prescaler value");
            return 0;
        }
    }

    FORCE_INLINE static float GetIwdgTimeout(IWDG_HandleTypeDef *handle)
    {
        // 获取IWDG预分频值
        const auto prescaler = GetPrescaler(handle);
        APPKIT_RAISE_IF_NOT(prescaler != 0, "IWDG prescaler is zero");

        // 获取IWDG重载值
        const uint32_t reload = handle->Init.Reload;
        APPKIT_RAISE_IF_NOT(reload <= 0xFFF, "IWDG reload value out of range"); // 重载值必须在0到4095之间

        return static_cast<float>(prescaler * (reload + 1)) / 32000.0f;
    }

    STM32Iwdg::STM32Iwdg(const char *name, IWDG_HandleTypeDef &handle, size_t stack_depth)
        : handle_(&handle)
    {
        APPKIT_RAISE_IF_NOT(nullptr != name, "Name must not be null");

        // 检查是否重复创建
        APPKIT_RAISE_IF_NOT(nullptr == RamFs::Dev().Find(name), "Device already exists");

        // 喂狗
        HAL_IWDG_Refresh(handle_);

        // 初始化喂狗线程
        const float timeout_s = GetIwdgTimeout(handle_);
        APPKIT_RAISE_IF_NOT(timeout_s > 0.0f, "IWDG timeout must be greater than zero");
        auto_feed_time_ = Duration::From<second>(timeout_s / 4); // 实际上，MCU内部的RC频率会在30kHz到60kHz之间变化。(芯片手册)取最差情况半周期进行更新.
        APPKIT_LOG_DEBUG("IWDG timeout: %.2f seconds, auto feed time: %.2f seconds", timeout_s,
                         DurationCast<second, float>(auto_feed_time_));

        thread_.Create(ThreadFunc, this, "IWDG_Thread", stack_depth, osal::Thread::Priority::REALTIME);

        // 创建设备文件
        APPKIT_RAISE_IF_NOT(Check(Block::RegisterDevice(name)), "Failed to register device");
    }

    __RAM_FUNC void STM32Iwdg::ThreadFunc(STM32Iwdg *self)
    {
        const auto &auto_feed_time = self->auto_feed_time_;
        static auto last_feed = Clock::system_clock->Now();
        while (true)
        {
            if (self->UpdateMonitors())
            {
                HAL_IWDG_Refresh(self->handle_);
            }
            else
            {
                // 如果有被监视实例超时
                APPKIT_LOG_WARNING("Monitor timeout, skipping feed");
            }

            osal::this_thread::SleepUntil(last_feed, auto_feed_time);
        }
    }
} // namespace stm32

#endif // IWDG
