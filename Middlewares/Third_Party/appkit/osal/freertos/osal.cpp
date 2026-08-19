#include <osal.hpp>
#include <osal_thread.hpp>

#include <common_assert.hpp>
#include <common_time.hpp>
#include <logger.hpp>

#include <platform.hpp>

extern void app_main(int argc, char *argv[]);

namespace appkit::osal
{
    static bool osalInitFinished = false;

    /**
     * @brief 系统时钟类
     */
    class SystemClock final : public Clock
    {
    public:
        constexpr SystemClock()
            : Clock(false)
        {
        }

        [[gnu::optimize("O2")]] TimePoint Now() const override
        {
            size_t tick = CheckInIsr() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();

            return TimePoint{Duration::From<SystemTimeUnit>(tick)};
        }
    };

    bool CheckInIsr()
    {
        return xPortIsInsideInterrupt() != 0;
    }

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
        xTaskCatchUpTicks(DurationCast<SystemTimeUnit, TickType_t>(time_point - Clock::system_clock->Now()));

        APPKIT_LOG_INFO("System time set successfully");
        return ErrorCode::OK;
    }
} // namespace appkit::osal

// 入口函数
extern "C"
{
    // 默认任务函数
    void StartDefaultTask(void const *argument)
    {
        using namespace appkit;

        UNUSED(argument);

        // 初始化系统内核
        APPKIT_RAISE_IF_NOT(Check(Platform::Init()), "Failed to initialize platform");
        APPKIT_RAISE_IF_NOT(Check(osal::Init()), "Failed to initialize OSAL subsystem");
        osal::osalInitFinished = true;

        // 调用用户主函数
        APPKIT_LOG_INFO("Start user main function");

        static char arg[6] = {"stm32"};
        static char *argv[2] = {arg, nullptr};
        app_main(1, argv);

        APPKIT_LOG_INFO("User main function exited");
    }

    // 堆栈溢出钩子函数
    void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
    {
        UNUSED(xTask);
        UNUSED(pcTaskName);

        APPKIT_RAISE_FROM_CALLBACK(true, "Stack overflow detected");
    }
} // extern "C"

[[gnu::weak]] void app_main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    APPKIT_LOG_WARNING("User app_main function is not implemented");
}

[[gnu::optimize("O2")]] void *operator new(size_t size) noexcept
{
    if (appkit::osal::osalInitFinished)
    {
        APPKIT_LOG_DEBUG("Allocating %u bytes of memory", size);
    }
    return pvPortMalloc(size);
}

[[gnu::optimize("O2")]] void *operator new[](size_t size) noexcept
{
    if (appkit::osal::osalInitFinished)
    {
        APPKIT_LOG_DEBUG("Allocating %u bytes of memory (array)", size);
    }
    return pvPortMalloc(size);
}

[[gnu::optimize("O2")]] void operator delete(void *ptr) noexcept
{
    if (appkit::osal::osalInitFinished)
    {
        APPKIT_LOG_DEBUG("Freeing memory at %p", ptr);
    }
    vPortFree(ptr);
}

[[gnu::optimize("O2")]] void operator delete[](void *ptr) noexcept
{
    if (appkit::osal::osalInitFinished)
    {
        APPKIT_LOG_DEBUG("Freeing memory at %p (array)", ptr);
    }
    vPortFree(ptr);
}