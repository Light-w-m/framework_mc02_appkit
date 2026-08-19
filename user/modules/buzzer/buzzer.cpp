#include <buzzer.hpp>

#include <logger.hpp>
#include <osal_thread.hpp>

Buzzer::Buzzer(const char *pwm_name)
{
    // 初始化PWM
    APPKIT_RAISE_IF_NOT(appkit::Check(pwm_.Open(pwm_name)),
                        "Failed to open PWM device");
}

appkit::ErrorCode Buzzer::Beep(float frequency, appkit::Duration duration)
{
    // 设置频率
    if (const auto ret = pwm_.SetFrequency(frequency);
        !appkit::Check(ret))
    {
        APPKIT_LOG_ERROR("Failed to set PWM frequency");
        return ret;
    }

    // 设置占空比为50%
    if (const auto ret = pwm_.SetDutyCycle(0.5f);
        !appkit::Check(ret))
    {
        APPKIT_LOG_ERROR("Failed to set PWM duty cycle");
        return ret;
    }

    // 启用PWM输出
    if (const auto ret = pwm_.Enable();
        !appkit::Check(ret))
    {
        APPKIT_LOG_ERROR("Failed to enable PWM output");
        return ret;
    }

    // 持续指定时间
    appkit::osal::this_thread::SleepFor(duration);

    // 禁用PWM输出
    if (const auto ret = pwm_.Disable();
        !appkit::Check(ret))
    {
        APPKIT_LOG_ERROR("Failed to disable PWM output");
        return ret;
    }

    return appkit::ErrorCode::OK;
}