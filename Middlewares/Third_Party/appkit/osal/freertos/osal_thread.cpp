/**
 * @file osal_thread.cpp
 * @brief OSAL线程实现文件
 * @author dusk
 */
#include <osal_thread.hpp>

#include <logger.hpp>

namespace appkit::osal
{
    FORCE_INLINE static size_t GetRtosPriority(Thread::Priority priority)
    {
        static const constinit auto step = (configMAX_PRIORITIES - 1) / 5;
        return std::to_underlying(priority) * step;
    }

    ErrorCode Thread::Create_(void *thread_base, void (*func)(void *), const char *name, const size_t stack_depth,
                              const Priority priority)
    {
        const auto stack_size = (stack_depth + 3) / 4;
        const auto real_priority = GetRtosPriority(priority);

        if (const auto ret = xTaskCreate(func, name, stack_size, thread_base, real_priority, std::addressof(this->handle_));
            ret != pdPASS)
        {
            // 触发警告
            APPKIT_LOG_ERROR("Thread <%s> creation failed", name);

            return ErrorCode::FAILED;
        }

        APPKIT_LOG_DEBUG("Thread <%s> created", name);
        return ErrorCode::OK;
    }

    void Thread::Exit_()
    {
        vTaskDelete(nullptr);
    }

    bool Thread::IsValid() const
    {
        return handle_ != nullptr;
    }

    Thread::operator ThreadId() const
    {
        return handle_;
    }

    namespace this_thread
    {
        Thread Current()
        {
            return Thread{xTaskGetCurrentTaskHandle()};
        }

        void SleepFor(const Duration &interval)
        {
            if (interval >= gWaitForever)
            {
                vTaskSuspend(nullptr);
            }
            else if (const auto ms = DurationCast<SystemTimeUnit, TickType_t>(interval);
                     ms >= 0)
            {
                vTaskDelay(ms);
            }
        }

        void SleepUntil(TimePoint &last_time, const Duration &interval)
        {
            if (interval >= gWaitForever)
            {
                vTaskSuspend(nullptr);
                return;
            }

            const auto now = Clock::system_clock->Now();

            if (const auto ms = DurationCast<SystemTimeUnit, TickType_t>(last_time + interval - now);
                ms == 0)
            {
                last_time = now;
            }
            else
            {
                vTaskDelay(ms);
                last_time += interval;
            }
        }

        void Yield()
        {
            if (!xPortIsInsideInterrupt())
            {
                portYIELD();
            }
        }
    }
}