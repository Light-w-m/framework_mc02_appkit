#include <application.hpp>

#include <ws2812.hpp>

#include <attitude.hpp>

using namespace application;

void ApplicationInit()
{
    static led::WS2812 ws2812{"spi6_led"};

    ws2812.SetColor(255, 0, 0);

    // 初始化姿态应用
    AttitudeInit();
}