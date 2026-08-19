#include <osal_semaphore.hpp>

#include <common_assert.hpp>

namespace appkit::osal
{
    Semaphore::Semaphore(const uint32_t initial_value)
    {
        APPKIT_RAISE_IF_NOT(sem_init(&handle_, 0, initial_value) == 0, "Semaphore creation failed");
    }

    Semaphore::~Semaphore()
    {
        sem_destroy(&handle_);
    }

    void Semaphore::Post()
    {
        sem_post(&handle_);
    }

    void Semaphore::PostFromCallback(bool in_isr)
    {
        UNUSED(in_isr);
        sem_post(&handle_);
    }

    ErrorCode Semaphore::Wait(const Duration &timeout)
    {
        timespec ts{};
        const auto next_time = Clock::system_clock->Now() + timeout;
        ts.tv_sec = DurationCast<second, decltype(ts.tv_sec)>(next_time.SinceEpoch());
        auto sec = Duration::From<second>(ts.tv_sec);
        ts.tv_nsec = DurationCast<nanosecond, decltype(ts.tv_nsec)>(next_time.SinceEpoch() - Duration::From<second>(ts.tv_sec));

        while (true)
        {
            const int ret = sem_clockwait(&handle_, CLOCK_REALTIME, &ts);
            if (ret == 0)
            {
                return ErrorCode::OK;
            }
            else if (ret == -1)
            {
                if (errno == ETIMEDOUT)
                {
                    // 超时
                    return ErrorCode::TIMEOUT;
                }
                else if (errno == EINTR)
                {
                    // 被信号中断，重试
                    continue;
                }
                else
                {
                    // 其他错误
                    return ErrorCode::FAILED;
                }
            }
        }
    }

    uint32_t Semaphore::GetValue()
    {
        int value{};
        if (sem_getvalue(&handle_, &value) == 0)
        {
            return static_cast<uint32_t>(value);
        }
        return 0;
    }
}