#include <hardware.hpp>

#include <stm32_power.hpp>
#include <stm32_rng.hpp>
#include <stm32_can.hpp>
#include <stm32_fdcan.hpp>
#include <stm32_uart.hpp>
#include <stm32_iwdg.hpp>
#include <stm32_gpio.hpp>
#include <stm32_timer.hpp>
#include <stm32_spi.hpp>
#include <stm32_adc.hpp>

#define BUFFER_SECTION(_section) [[using gnu: section(_section), used]]

/* UART 设备缓冲区 */
BUFFER_SECTION(".SRAM1") static uint8_t uart1_tx_buffer[512];
BUFFER_SECTION(".SRAM1") static uint8_t uart1_rx_buffer[128];

BUFFER_SECTION(".SRAM1") static uint8_t uart2_tx_buffer[128];
BUFFER_SECTION(".SRAM1") static uint8_t uart2_rx_buffer[128];

BUFFER_SECTION(".SRAM1") static uint8_t uart3_tx_buffer[128];
BUFFER_SECTION(".SRAM1") static uint8_t uart3_rx_buffer[128];

BUFFER_SECTION(".SRAM1") static uint8_t uart5_rx_buffer[128];

BUFFER_SECTION(".SRAM1") static uint8_t uart7_tx_buffer[128];
BUFFER_SECTION(".SRAM1") static uint8_t uart7_rx_buffer[128];

BUFFER_SECTION(".SRAM1") static uint8_t uart10_tx_buffer[128];
BUFFER_SECTION(".SRAM1") static uint8_t uart10_rx_buffer[128];

/* SPI 设备缓冲区 */
BUFFER_SECTION(".SRAM1") static uint8_t spi2_tx_buffer[128];
BUFFER_SECTION(".SRAM1") static uint8_t spi2_rx_buffer[128];

BUFFER_SECTION(".SRAM3") static uint8_t spi6_tx_buffer[128];

/* ADC 设备缓冲区 */
BUFFER_SECTION(".SRAM3") static uint8_t adc1_buffer[16];

void HardwareInit()
{
    using namespace appkit;
    using namespace appkit::stm32;

    // timebase
    static STM32Timer timebase{"timebase", htim5, true};
    Clock::steady_clock = &timebase; // 配置默认时钟

    // iwdg
    static STM32Iwdg iwdg{"iwdg", hiwdg1};
    // pwr
    static STM32Power power_manager{};
    // gpio
    static STM32GPIO gpio_key{"user_key", KEY_GPIO_Port, KEY_Pin};

    static STM32GPIO gpio_cs0_accel{"cs_accel", SPI2_CS0_GPIO_Port, SPI2_CS0_Pin};
    static STM32GPIO gpio_cs0_gyro{"cs_gyro", SPI2_CS1_GPIO_Port, SPI2_CS1_Pin};
    static STM32GPIO gpio_int0_accel{"int0_accel", INT_ACC_GPIO_Port, INT_ACC_Pin, EXTI15_10_IRQn};
    static STM32GPIO gpio_int0_gyro{"int0_gyro", INT_GYRO_GPIO_Port, INT_GYRO_Pin, EXTI15_10_IRQn};
    // adc
    static STM32ADC adc1{hadc1, RawData{adc1_buffer}, 3.3f};
    static STM32ADC::Channel adc1_ch0{"adc1_ch4", adc1, 0, 11.0f, 0.0f}; // VCCIN
    // rng
    static STM32RNG rng{"rng", hrng};
    // FDCAN
    FDCAN_MsgRAMCheck(hfdcan1, hfdcan2, hfdcan3); // 检查FDCAN MsgBuffer配置

    static STM32FDCAN fdcan1{"fdcan1", hfdcan1, 8, 8};
    static STM32FDCAN fdcan2{"fdcan2", hfdcan2, 8, 8};
    static STM32FDCAN fdcan3{"fdcan3", hfdcan3, 8, 8};
    // uart
    static STM32Uart uart1{"uart1", huart1, RawData{uart1_tx_buffer}, RawData{uart1_rx_buffer}};
    static STM32Uart uart2{"uart2", huart2, RawData{uart2_tx_buffer}, RawData{uart2_rx_buffer}};
    static STM32Uart uart3{"uart3", huart3, RawData{uart3_tx_buffer}, RawData{uart3_rx_buffer}};
    static STM32Uart uart5{"uart5", huart5, RawData{}, RawData{uart5_rx_buffer}, 0};
    static STM32Uart uart7{"uart7", huart7, RawData{uart7_tx_buffer}, RawData{uart7_rx_buffer}};
    static STM32Uart uart10{"uart10", huart10, RawData{uart10_tx_buffer}, RawData{uart10_rx_buffer}};

    STDIO::read_ = uart1.read;
    STDIO::write_ = uart1.write;
    // pwm
    static STM32Timer timer_tim1{"timer1", htim1};
    static STM32Timer::Channel pwm_tim1_ch1{"timer1_ch1", timer_tim1, TIM_CHANNEL_1};
    static STM32Timer::Channel pwm_tim1_ch3{"timer1_ch3", timer_tim1, TIM_CHANNEL_3};

    static STM32Timer timer_tim2{"timer2", htim2};
    static STM32Timer::Channel pwm_tim2_ch1{"timer2_ch1", timer_tim2, TIM_CHANNEL_1};
    static STM32Timer::Channel pwm_tim2_ch3{"timer2_ch3", timer_tim2, TIM_CHANNEL_3};

    static STM32Timer timer_tim3{"timer3", htim3};
    static STM32Timer::Channel pwm_tim3_ch4{"timer3_ch4", timer_tim3, TIM_CHANNEL_4};

    static STM32Timer timer_tim12{"timer12", htim12};
    static STM32Timer::Channel pwm_tim12_ch2{"timer12_ch2", timer_tim12, TIM_CHANNEL_2};
    // spi
    static STM32SPI spi2{hspi2, RawData{spi2_tx_buffer}, RawData{spi2_rx_buffer}, 4};
    static STM32SPI::Subset spi2_accel{spi2, "spi2_accel", "cs_accel"};
    static STM32SPI::Subset spi2_gyro{spi2, "spi2_gyro", "cs_gyro"};

    static STM32SPI spi6{hspi6, RawData{spi6_tx_buffer}, RawData{}};
    static STM32SPI::Subset spi6_led{spi6, "spi6_led", nullptr};
}