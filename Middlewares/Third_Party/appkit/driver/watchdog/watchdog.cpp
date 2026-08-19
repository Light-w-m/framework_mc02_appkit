#include <watchdog.hpp>

#include <logger.hpp>
#include <osal_thread.hpp>

namespace appkit
{
    Watchdog::~Watchdog()
    {
        if (block_)
        {
            block_->monitor_list_.Delete(monitor_node_);
        }
    }

    ErrorCode Watchdog::Open(const char *name, const Duration &timeout)
    {
        if (nullptr == name) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Invalid device name");
            return ErrorCode::INVALID_ARG;
        }

        if (nullptr != block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Device already open");
            return ErrorCode::FAILED;
        }

        if (const auto ret = Block::GetDeviceBlock(name, &block_);
            !Check(ret)) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Failed to get device block");
            return ret;
        }

        if (timeout < block_->auto_feed_time_) [[unlikely]]
        {
            APPKIT_LOG_WARNING("timeout %dms less than auto feed time %dms",
                               DurationCast<millisecond, int32_t>(timeout),
                               DurationCast<millisecond, int32_t>(block_->auto_feed_time_));
        }

        monitor_node_ = {timeout, Clock::system_clock->Now() + timeout};
        block_->monitor_list_.Add(monitor_node_);

        return ErrorCode::OK;
    }

    void Watchdog::SetTimeout(const Duration &time)
    {
        if (block_)
        {
            (*monitor_node_).timeout_ = time;
            (*monitor_node_).next_feed_ = Clock::system_clock->Now() + time;
        }
    }

    void Watchdog::Feed()
    {
        if (block_)
        {
            (*monitor_node_).next_feed_ = Clock::system_clock->Now() + (*monitor_node_).timeout_;
        }
    }

    bool Watchdog::Block::UpdateMonitors()
    {
        bool ret = true;
        const auto now = Clock::system_clock->Now();

        UNUSED(monitor_list_.ForEach<Monitor>(
            [&ret, now](Monitor &monitor) -> ErrorCode
            {
                if (now >= monitor.next_feed_)
                {
                    ret = false;
                }
                return ErrorCode::OK;
            }));
        return ret;
    }
}
