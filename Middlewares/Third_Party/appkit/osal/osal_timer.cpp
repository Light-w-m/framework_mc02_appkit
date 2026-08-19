/**
 * @file osal_timer.cpp
 * @brief OSAL定时器实现
 * @author dusk
 */
#include <osal_timer.hpp>

#include <logger.hpp>

#ifndef APPKIT_OSAL_TIMER_STACK_DEPTH
#pragma message "APPKIT_OSAL_TIMER_STACK_DEPTH is not defined, using default value 1024"
#define APPKIT_OSAL_TIMER_STACK_DEPTH 1024
#endif

#ifndef APPKIT_OSAL_TIMER_PRIORITY
#pragma message "APPKIT_OSAL_TIMER_PRIORITY is not defined, using default value NORMAL"
#define APPKIT_OSAL_TIMER_PRIORITY NORMAL
#endif

#ifndef APPKIT_OSAL_TIMER_REFRESH_INTERVAL
#pragma message "APPKIT_OSAL_TIMER_REFRESH_INTERVAL is not defined, using default value 1"
#define APPKIT_OSAL_TIMER_REFRESH_INTERVAL 1
#endif

namespace appkit::osal
{
    constinit Thread Timer::timer_thread_{};
    constinit LockFreeList Timer::list_{};

    constinit uint32_t Timer::stack_depth{APPKIT_OSAL_TIMER_STACK_DEPTH};
    constinit Thread::Priority Timer::priority{Thread::Priority::APPKIT_OSAL_TIMER_PRIORITY};

    constinit Duration Timer::interval_{Duration::From<millisecond>(APPKIT_OSAL_TIMER_REFRESH_INTERVAL)};

    void Timer::ReflashFunc()
    {
        APPKIT_LOG_DEBUG("Appkit timer thread started with interval %u ms", interval_.To<millisecond>());
        auto time = Clock::system_clock->Now();
        while (true)
        {
            Reflash();
            this_thread::SleepUntil(time, interval_);
        }
    }

    ErrorCode Timer::Start()
    {
        if (!timer_thread_.IsValid())
        {
            return timer_thread_.Create(ReflashFunc, "appkit_timer_task", stack_depth, priority);
        }
        return ErrorCode::BUSY;
    }

    void Timer::Reflash()
    {
        static constinit auto func = [](TaskInfo& info)
        {
            if (info.enabled)
            {
                if (++info.count_ >= info.period)
                {
                    info.count_ = 0;
                    info.Run();
                }
            }
            return ErrorCode::OK;
        };

        list_.ForEach<TaskInfo>(func);
    }
}