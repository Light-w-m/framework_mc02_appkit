#include <stm32_power.hpp>

#include <main.h>
#include <logger.hpp>

namespace appkit::stm32
{
    STM32Power::STM32Power()
    {
        // 全局信号量初始化
        reset_event_.Open("reset");
        shutdown_event_.Open("shutdown");
        jump_event_.Open("jump");
        
        // 注册设备文件
        APPKIT_RAISE_IF_NOT(Check(Block::RegisterDevice("power")), "failed to register power device");
    }

    void STM32Power::ChangeStatus(const Status status)
    {
        switch (status)
        {
        case Status::Reset:
        {
            reset_event_.Activate(0x00);
            Reset();
            break;
        }
        case Status::Shutdown:
        {
            shutdown_event_.Activate(0x00);
            Shutdown();
            break;
        }
        case Status::JumpToBootloader:
        {
            jump_event_.Activate(0x00);
            JumpToBootloader();
            break;
        }
        }
    }

    void STM32Power::Reset()
    {
        HAL_NVIC_SystemReset();
    }

    void STM32Power::Shutdown()
    {
        HAL_PWR_EnterSTANDBYMode();
    }

    void STM32Power::JumpToBootloader()
    {
        Reset();
    }
}
