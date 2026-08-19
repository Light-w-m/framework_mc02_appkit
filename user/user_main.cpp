#include <hardware.hpp>
#include <application.hpp>

#include <osal_thread.hpp>

#include <container.hpp>
#include <logger.hpp>

using namespace appkit::time_literals;

#include <butterworth_filter.hpp>
#include <eskf.hpp>

void app_main(int argc, char *argv[])
{
    HardwareInit();
    ApplicationInit();

    algorithm::ButterworthFilter<algorithm::ButterworthFilterType::BANDPASS> filter(1000.0f, 100.0f, 300.0f);

    int a{};
    float b;

    appkit::Container container(
        appkit::Entry<int>(a, "variable_a"),
        appkit::Entry<float>(b, "variable_b"));

    for (;;)
    {
        if (auto result = container.Find<int>("variable_a");
            result)
        {
            // 成功获取到变量a的引用
            *result.GetValue() += 1;
            // APPKIT_LOG_INFO("Variable a incremented to %d", *result.GetValue());
        }

        // int *test = new int(42);
        appkit::osal::this_thread::SleepFor(1_s);
        // delete test;
    }
}