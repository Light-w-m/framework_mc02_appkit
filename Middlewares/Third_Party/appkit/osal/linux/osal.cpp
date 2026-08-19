#include <osal.hpp>

#include <common_assert.hpp>
#include <common_time.hpp>
#include <logger.hpp>

#include <platform.hpp>

extern void app_main(int argc, char *argv[]);

namespace appkit::osal
{
    class SystemClock final : public Clock
    {
    public:
        constexpr SystemClock()
            : Clock(false)
        {
        }

        TimePoint Now() const override
        {
            thread_local struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            return TimePoint{Duration::From<second>(ts.tv_sec) + Duration::From<nanosecond>(ts.tv_nsec)};
        }
    };

    ErrorCode Init()
    {
        // 初始化系统时间
        static constinit SystemClock system_clock{};
        Clock::system_clock = std::addressof(system_clock);

        APPKIT_LOG_INFO("OSAL subsystem initialized");
        return ErrorCode::OK;
    }

    ErrorCode SetTime(const TimePoint &time_point)
    {
        struct timespec ts;
        ts.tv_sec = DurationCast<second, time_t>(time_point.SinceEpoch());
        ts.tv_nsec = DurationCast<nanosecond, long>(time_point.SinceEpoch() - Duration::From<second>(ts.tv_sec));

        if (clock_settime(CLOCK_REALTIME, &ts) != 0)
        {
            APPKIT_LOG_ERROR("Failed to set system time (%s)", strerror(errno));
            return ErrorCode::FAILED;
        }

        APPKIT_LOG_INFO("System time set successfully");
        return ErrorCode::OK;
    }
} // namespace appkit::osal

// 入口函数
int main(int argc, char *argv[])
{
    using namespace appkit;

    // 初始化系统内核
    APPKIT_RAISE_IF_NOT(Check(Platform::Init()), "Failed to initialize platform");
    APPKIT_RAISE_IF_NOT(Check(osal::Init()), "Failed to initialize OSAL subsystem");

    // 调用用户主函数
    APPKIT_LOG_INFO("Start user main function");
    app_main(argc, argv);
    APPKIT_LOG_INFO("User main function exited");

    return 0;
}