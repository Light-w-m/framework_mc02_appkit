#pragma once

#include <power.hpp>
#include <event.hpp>

namespace appkit::stm32
{
    class STM32Power final : public PowerManager::Block
    {
        Event reset_event_;
        Event shutdown_event_;
        Event jump_event_;

        void ChangeStatus(Status status) override;

    public:
        STM32Power();

        static void Reset();

        static void Shutdown();

        static void JumpToBootloader();
    };
}
