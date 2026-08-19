#include <linux_power.hpp>

#include <logger.hpp>

#include <unistd.h>
#include <sys/reboot.h>

namespace appkit
{
    LinuxPower::LinuxPower()
    {
        APPKIT_RAISE_IF_NOT(Check(RegisterDevice("power")),
                            "Failed to register Linux power device");

        // 全局信号量初始化
        reset_event_.Open("reset");
        shutdown_event_.Open("shutdown");
    }

    void LinuxPower::ChangeStatus(PowerManager::Block::Status status)
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
            APPKIT_LOG_WARNING("Jump to bootloader is not supported on Linux platform");
            break;
        }
        default:
            APPKIT_RAISE("Invalid power status");
        }
    }

    void LinuxPower::CheckRoot()
    {
        if (geteuid() != 0)
        {
            APPKIT_LOG_ERROR("Root privileges are required to perform this operation");
        }
    }

    void LinuxPower::Reset()
    {
        CheckRoot();
        if (::system("reboot") != 0)
        {
            APPKIT_LOG_ERROR("Failed to reboot the system");
        }
    }

    void LinuxPower::Shutdown()
    {
        CheckRoot();
        if (::system("poweroff") != 0)
        {
            APPKIT_LOG_ERROR("Failed to shutdown the system");
        }
    }
} // namespace appkit