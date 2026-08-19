#pragma once

#include <pwm.hpp>

/**
 * @brief 蜂鸣器模块类
 */
class Buzzer final
{
private:
    appkit::PWM pwm_{}; ///< PWM对象

public:
    /**
     * @brief 构造函数
     * @param pwm_name PWM设备名
     */
    Buzzer(const char *pwm_name);

    /**
     * @brief 蜂鸣函数
     * @param frequency 频率，单位：Hz
     * @param duration 持续时间
     * @return 错误码
     */
    appkit::ErrorCode Beep(float frequency, appkit::Duration duration);
};