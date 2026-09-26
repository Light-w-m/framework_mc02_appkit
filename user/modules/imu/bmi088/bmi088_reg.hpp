#pragma once

#ifdef ENABLED_MODULES_BMI088

#include <matrix.hpp>

namespace sensor::imu
{
    /**
     * @brief BMI088寄存器定义
     */
    struct BMI088Def final
    {
        /**
         * @brief 加速度计寄存器定义
         */
        struct Accel final
        {
            /**
             * @brief 过采样枚举定义
             */
            enum class OverSampling : uint8_t
            {
                OSR_1 = 0x02, ///< 1倍过采样
                OSR_2 = 0x01, ///< 2倍过采样
                OSR_4 = 0x00  ///< 4倍过采样
            };

            /**
             * @brief 输出数据率枚举定义
             */
            enum class OutputDataRate : uint8_t
            {
                ODR_12_5HZ = 0x05, ///< 12.5Hz输出数据率
                ODR_25HZ = 0x06,   ///< 25Hz输出数据率
                ODR_50HZ = 0x07,   ///< 50Hz输出数据率
                ODR_100HZ = 0x08,  ///< 100Hz输出数据率
                ODR_200HZ = 0x09,  ///< 200Hz输出数据率
                ODR_400HZ = 0x0A,  ///< 400Hz输出数据率
                ODR_800HZ = 0x0B,  ///< 800Hz输出数据率
                ODR_1600HZ = 0x0C  ///< 1600Hz输出数据率
            };

            /**
             * @brief 量程枚举定义
             */
            enum class Range : uint8_t
            {
                RANGE_3G = 0x00,  ///< ±3g量程
                RANGE_6G = 0x01,  ///< ±6g量程
                RANGE_12G = 0x02, ///< ±12g量程
                RANGE_24G = 0x03  ///< ±24g量程
            };

            /**
             * @brief GPIO中断引脚枚举定义
             */
            enum class GpioIntPin : uint8_t
            {
                INT1 = 0x00, ///< GPIO中断引脚1
                INT2 = 0x01  ///< GPIO中断引脚2
            };

            /**
             * @brief GPIO中断模式枚举定义
             */
            enum class GpioIntMode : uint8_t
            {
                PUSH_PULL = 0x00, ///< 推挽输出
                OPEN_DRAIN = 0x01 ///< 开漏输出
            };

            /**
             * @brief GPIO中断电平枚举定义
             */
            enum class GpioIntLevel : uint8_t
            {
                ACTIVE_HIGH = 0x00, ///< 高电平有效
                ACTIVE_LOW = 0x01   ///< 低电平有效
            };

            /**
             * @brief GPIO中断配置结构体定义
             */
            struct GpioInt final
            {
                uint8_t ctrl_reg;  ///< GPIO中断控制寄存器
                uint8_t drdy_mask; ///< 数据就绪掩码

                /**
                 * @brief 获取中断控制寄存器值
                 * @param enable 是否使能中断
                 * @param mode 中断模式
                 * @param level 中断电平
                 * @return 控制寄存器值
                 */
                static constexpr uint8_t GetCtrlValue(bool enable, GpioIntMode mode, GpioIntLevel level)
                {
                    return (static_cast<uint8_t>(enable) << 0x03) |
                           (static_cast<uint8_t>(mode) << 0x02) |
                           (static_cast<uint8_t>(level) << 0x01);
                }
            };

            static inline constexpr uint8_t CHIP_ID_REG = 0x00;   ///< 加速度计芯片ID寄存器
            static inline constexpr uint8_t CHIP_ID_VALUE = 0x1E; ///< 加速度计芯片ID寄存器值

            static inline constexpr uint8_t ERR_REG = 0x02;                ///< 加速度计错误寄存器
            static inline constexpr uint8_t CONGIF_ERROR_MASK = 1 << 0x02; ///< 加速度计错误寄存器掩码
            static inline constexpr uint8_t FATAL_ERROR_MASK = 1 << 0x00;  ///< 致命错误寄存器掩码

            static inline constexpr uint8_t STATUS_REG = 0x03;     ///< 加速度计电源状态寄存器
            static inline constexpr uint8_t DRDY_MASK = 1 << 0x07; ///< 加速度计数据就绪掩码

            static inline constexpr uint8_t DATA_X_L_REG = 0x12; ///< 加速度计X轴数据低字节寄存器
            static inline constexpr uint8_t DATA_X_H_REG = 0x13; ///< 加速度计X轴数据高字节寄存器
            static inline constexpr uint8_t DATA_Y_L_REG = 0x14; ///< 加速度计Y轴数据低字节寄存器
            static inline constexpr uint8_t DATA_Y_H_REG = 0x15; ///< 加速度计Y轴数据高字节寄存器
            static inline constexpr uint8_t DATA_Z_L_REG = 0x16; ///< 加速度计Z轴数据低字节寄存器
            static inline constexpr uint8_t DATA_Z_H_REG = 0x17; ///< 加速度计Z轴数据高字节寄存器

            /**
             * @brief 获取加速度值
             * @param raw_data 原始加速度数据
             * @param range 量程设置
             * @return 加速度值（单位：g）
             */
            static constexpr float GetAccData(int16_t raw_data, Range range)
            {
                static constexpr const float scale_factor[4] = {3.0f / 32768.0f, 6.0f / 32768.0f, 12.0f / 32768.0f, 24.0f / 32768.0f};

                return raw_data * scale_factor[static_cast<uint8_t>(range)];
            }

            static inline constexpr uint8_t SENSORTIME_L_REG = 0x18; ///< 加速度计传感器时间低字节寄存器
            static inline constexpr uint8_t SENSORTIME_M_REG = 0x19; ///< 加速度计传感器时间中字节寄存器
            static inline constexpr uint8_t SENSORTIME_H_REG = 0x1A; ///< 加速度计传感器时间高字节寄存器

            static inline constexpr uint8_t INT_STATUS_REG = 0x1D;     ///< 加速度计中断状态寄存器
            static inline constexpr uint8_t DRDY_INT_MASK = 1 << 0x07; ///< 数据就绪中断掩码

            static inline constexpr uint8_t TEMP_H_REG = 0x22; ///< 加速度计温度高字节寄存器
            static inline constexpr uint8_t TEMP_L_REG = 0x23; ///< 加速度计温度低字节寄存器

            /**
             * @brief 获取加速度计温度值
             * @param temp_high 温度高字节
             * @param temp_low 温度低字节
             * @return 温度值（单位：摄氏度）
             */
            static constexpr float GetTemperature(uint8_t temp_high, uint8_t temp_low)
            {
                int16_t temp_raw = (static_cast<int16_t>(temp_high) << 3) | (static_cast<int16_t>(temp_low) >> 5);
                if (temp_raw > 1023)
                {
                    // 负数处理
                    temp_raw -= 2048;
                }

                return 23.0f + temp_raw * 0.125f;
            }

            static inline constexpr uint8_t ACC_CONF_REG = 0x40;          ///< 加速度计配置寄存器
            static inline constexpr uint8_t ACC_CONF_MUSTSET_MASK = 0x80; ///< 加速度计配置寄存器必须设置掩码

            /**
             * @brief 获取加速度计配置寄存器值
             * @param osr 过采样设置
             * @param odr 输出数据率设置
             * @return 配置寄存器值
             */
            static constexpr uint8_t GetAccConfValue(OverSampling osr, OutputDataRate odr)
            {
                return ACC_CONF_MUSTSET_MASK | (static_cast<uint8_t>(osr) << 0x04) | static_cast<uint8_t>(odr);
            }

            static inline constexpr uint8_t ACC_RANGE_REG = 0x41; ///< 加速度计量程寄存器

            /**
             * @brief 获取加速度计量程寄存器值
             * @param range 量程设置
             * @return 量程寄存器值
             */
            static constexpr uint8_t GetAccRangeValue(Range range)
            {
                return static_cast<uint8_t>(range);
            }

            static inline constexpr uint8_t INT_MAP_REG = 0x55; ///< 加速度计中断映射寄存器
            static inline constexpr GpioInt int_settings[2] = {
                // INT1配置
                {
                    .ctrl_reg = 0x53,
                    .drdy_mask = 1 << 0x02},
                // INT2配置
                {
                    .ctrl_reg = 0x54,
                    .drdy_mask = 1 << 0x06}}; ///< 加速度计GPIO中断配置

            static inline constexpr uint8_t SELF_TEST_REG = 0x6D;             ///< 加速度计自检寄存器
            static inline constexpr uint8_t SELF_TEST_OFF = 1 << 0x07;        ///< 加速度计自检关闭
            static inline constexpr uint8_t SELF_TEST_POSITIVE_SIGNAL = 0x0D; ///< 加速度计自检正信号
            static inline constexpr uint8_t SELF_TEST_NEGATIVE_SIGNAL = 0x09; ///< 加速度计自检负信号

            static inline constexpr uint8_t PWR_CONF_REG = 0x7C;     ///< 加速度计电源配置寄存器
            static inline constexpr uint8_t PWR_SUSPEND_MODE = 0x03; ///< 加速度计悬挂模式
            static inline constexpr uint8_t PWR_ACTIVE_MODE = 0x00;  ///< 加速度计活动模式

            static inline constexpr uint8_t PWR_CTRL_REG = 0x7D;       ///< 加速度计电源控制寄存器
            static inline constexpr uint8_t PWR_ENABLE_ACC_OFF = 0x00; ///< 加速度计关闭
            static inline constexpr uint8_t PWR_ENABLE_ACC_ON = 0x04;  ///< 加速度计开启

            static inline constexpr uint8_t SOFTRESET_REG = 0x7E;   ///< 加速度计软复位寄存器
            static inline constexpr uint8_t SOFTRESET_VALUE = 0xB6; ///< 加速度计软复位命令

            /**
             * @brief 获取实际输出数据率
             * @param odr 输出数据率枚举值
             * @return 实际输出数据率（单位：Hz）
             */
            static constexpr float GetRealOutputDataRate(OutputDataRate odr)
            {
                switch (odr)
                {
                case OutputDataRate::ODR_12_5HZ:
                    return 12.5f;
                case OutputDataRate::ODR_25HZ:
                    return 25.0f;
                case OutputDataRate::ODR_50HZ:
                    return 50.0f;
                case OutputDataRate::ODR_100HZ:
                    return 100.0f;
                case OutputDataRate::ODR_200HZ:
                    return 200.0f;
                case OutputDataRate::ODR_400HZ:
                    return 400.0f;
                case OutputDataRate::ODR_800HZ:
                    return 800.0f;
                case OutputDataRate::ODR_1600HZ:
                    return 1600.0f;
                default:
                    return 0.0f;
                }
            }

            /**
             * @brief 获取带宽
             * @param odr 输出数据率枚举值
             * @param osr 过采样枚举值
             * @return 带宽（单位：Hz）
             */
            static constexpr appkit::math::Vectorf<3> GetBaudwidth(OutputDataRate odr, OverSampling osr)
            {
                switch (odr)
                {
                case OutputDataRate::ODR_12_5HZ:
                {
                    switch (osr)
                    {
                    case OverSampling::OSR_1:
                        return {5, 5, 5};
                    case OverSampling::OSR_2:
                        return {2, 2, 2};
                    case OverSampling::OSR_4:
                        return {1, 1, 1};
                    }
                }
                case OutputDataRate::ODR_25HZ:
                {
                    switch (osr)
                    {
                    case OverSampling::OSR_1:
                        return {10, 10, 10};
                    case OverSampling::OSR_2:
                        return {5, 5, 5};
                    case OverSampling::OSR_4:
                        return {3, 3, 3};
                    }
                }
                case OutputDataRate::ODR_50HZ:
                {
                    switch (osr)
                    {
                    case OverSampling::OSR_1:
                        return {20, 20, 20};
                    case OverSampling::OSR_2:
                        return {9, 9, 9};
                    case OverSampling::OSR_4:
                        return {5, 5, 5};
                    }
                }
                case OutputDataRate::ODR_100HZ:
                {
                    switch (osr)
                    {
                    case OverSampling::OSR_1:
                        return {40, 40, 40};
                    case OverSampling::OSR_2:
                        return {19, 19, 19};
                    case OverSampling::OSR_4:
                        return {10, 10, 10};
                    }
                }
                case OutputDataRate::ODR_200HZ:
                {
                    switch (osr)
                    {
                    case OverSampling::OSR_1:
                        return {80, 80, 80};
                    case OverSampling::OSR_2:
                        return {38, 38, 38};
                    case OverSampling::OSR_4:
                        return {20, 20, 20};
                    }
                }
                case OutputDataRate::ODR_400HZ:
                {
                    switch (osr)
                    {
                    case OverSampling::OSR_1:
                        return {145, 145, 145};
                    case OverSampling::OSR_2:
                        return {75, 75, 75};
                    case OverSampling::OSR_4:
                        return {40, 40, 40};
                    }
                }
                case OutputDataRate::ODR_800HZ:
                {
                    switch (osr)
                    {
                    case OverSampling::OSR_1:
                        return {230, 230, 200};
                    case OverSampling::OSR_2:
                        return {140, 140, 140};
                    case OverSampling::OSR_4:
                        return {80, 80, 80};
                    }
                }
                case OutputDataRate::ODR_1600HZ:
                {
                    switch (osr)
                    {
                    case OverSampling::OSR_1:
                        return {280, 280, 245};
                    case OverSampling::OSR_2:
                        return {234, 234, 215};
                    case OverSampling::OSR_4:
                        return {145, 145, 145};
                    }
                }
                default:
                    return {0, 0, 0};
                }
            }

            /**
             * @brief 获取默认协方差矩阵
             * @param odr 输出数据率枚举值
             * @param osr 过采样枚举值
             * @return 协方差矩阵，单位：(g²)
             */
            static constexpr appkit::math::Matrixf<3, 3> GetDefaultCovariance(OutputDataRate odr, OverSampling osr)
            {
                const auto baudwidth = GetBaudwidth(odr, osr);
                return appkit::math::diagonal_matrix<float, 3>(
                    (0.000160 * 0.000160) * sqrtf(baudwidth(0, 0)),
                    (0.000160 * 0.000160) * sqrtf(baudwidth(1, 0)),
                    (0.000190 * 0.000190) * sqrtf(baudwidth(2, 0)));
            }
        };

        /**
         * @brief 陀螺仪寄存器定义
         */
        struct Gyro final
        {
            /**
             * @brief 输出数据率枚举定义
             */
            enum class OutputDataRate : uint8_t
            {
                ODR_2000_532_HZ = 0x00, ///< 2000度每秒输出数据率532Hz，带宽230Hz
                ODR_2000_230_HZ = 0x01, ///< 2000度每秒输出数据率230Hz，带宽116Hz
                ODR_1000_116_HZ = 0x02, ///< 1000度每秒输出数据率116Hz，带宽47Hz
                ODR_400_47_HZ = 0x03,   ///< 400度每秒输出数据率47Hz，带宽23Hz
                ODR_200_23_HZ = 0x04,   ///< 200度每秒输出数据率23Hz，带宽12Hz
                ODR_100_12_HZ = 0x05,   ///< 100度每秒输出数据率12Hz，带宽6Hz
                ODR_200_64_HZ = 0x06,   ///< 200度每秒输出数据率64Hz，带宽32Hz
                ODR_100_32_HZ = 0x07    ///< 100度每秒输出数据率32Hz，带宽16Hz
            };

            /**
             * @brief 量程枚举定义
             */
            enum class Range : uint8_t
            {
                RANGE_2000DPS = 0x00, ///< ±2000度每秒量程
                RANGE_1000DPS = 0x01, ///< ±1000度每秒量程
                RANGE_500DPS = 0x02,  ///< ±500度每秒量程
                RANGE_250DPS = 0x03,  ///< ±250度每秒量程
                RANGE_125DPS = 0x04   ///< ±125度每秒量程
            };

            /**
             * @brief GPIO中断引脚枚举定义
             */
            enum class GpioIntPin : uint8_t
            {
                INT3 = 0x00, ///< GPIO中断引脚3
                INT4 = 0x01  ///< GPIO中断引脚4
            };

            /**
             * @brief GPIO中断模式枚举定义
             */
            using GpioIntMode = Accel::GpioIntMode;

            /**
             * @brief GPIO中断电平枚举定义
             */
            using GpioIntLevel = Accel::GpioIntLevel;

            /**
             * @brief GPIO中断配置结构体定义
             */
            struct GpioInt final
            {
                /**
                 * @brief 获取中断控制寄存器值
                 * @param enable 是否使能中断
                 * @param mode 中断模式
                 * @param level 中断电平
                 * @return 控制寄存器值
                 */
                static constexpr uint8_t GetConfValue(GpioIntPin pin, GpioIntMode mode, GpioIntLevel level)
                {
                    uint8_t reg_value = (static_cast<uint8_t>(mode) << 0x01) | static_cast<uint8_t>(level);

                    if (pin == GpioIntPin::INT4)
                    {
                        reg_value <<= 0x02; // INT4配置在高两位
                    }

                    return reg_value;
                }

                /**
                 * @brief 获取中断使能寄存器值
                 * @param pin3_enable 是否使能中断3
                 * @param pin4_enable 是否使能中断4
                 * @return 使能寄存器值
                 */
                static constexpr uint8_t GetEnableValue(bool pin3_enable, bool pin4_enable)
                {
                    return (static_cast<uint8_t>(pin3_enable) << 0x00) | // Enable INT3 0x01
                           (static_cast<uint8_t>(pin4_enable) << 0x07);  // Enable INT4 0x80
                }
            };

            static inline constexpr uint8_t CHIP_ID_REG = 0x00;   ///< 陀螺仪芯片ID寄存器
            static inline constexpr uint8_t CHIP_ID_VALUE = 0x0F; ///< 陀螺仪芯片ID寄存器值

            static inline constexpr uint8_t DATA_X_L_REG = 0x02; ///< 陀螺仪X轴数据低字节寄存器
            static inline constexpr uint8_t DATA_X_H_REG = 0x03; ///< 陀螺仪X轴数据高字节寄存器
            static inline constexpr uint8_t DATA_Y_L_REG = 0x04; ///< 陀螺仪Y轴数据低字节寄存器
            static inline constexpr uint8_t DATA_Y_H_REG = 0x05; ///< 陀螺仪Y轴数据高字节寄存器
            static inline constexpr uint8_t DATA_Z_L_REG = 0x06; ///< 陀螺仪Z轴数据低字节寄存器
            static inline constexpr uint8_t DATA_Z_H_REG = 0x07; ///< 陀螺仪Z轴数据高字节寄存器

            /**
             * @brief 获取角速度值
             * @param raw_data 原始角速度数据
             * @param range 量程设置
             * @return 角速度值（单位：度每秒）
             */
            static constexpr float GetGyroData(int16_t raw_data, Range range)
            {
                static constexpr const float scale_factor[5] = {
                    2000.0f / 32768.0f, 1000.0f / 32768.0f, 500.0f / 32768.0f, 250.0f / 32768.0f, 125.0f / 32768.0f};
                return raw_data * scale_factor[static_cast<uint8_t>(range)];
            }

            static inline constexpr uint8_t INT_STATUS_REG = 0x0A;     ///< 陀螺仪中断状态寄存器
            static inline constexpr uint8_t DRDY_INT_MASK = 1 << 0x07; ///< 数据就绪中断掩码

            static inline constexpr uint8_t RANGE_REG = 0x0F; ///< 陀螺仪量程寄存器

            /**
             * @brief 获取陀螺仪量程寄存器值
             * @param range 量程设置
             * @return 量程寄存器值
             */
            static constexpr uint8_t GetRangeValue(Range range)
            {
                return static_cast<uint8_t>(range);
            }

            static inline constexpr uint8_t BW_REG = 0x10;          ///< 陀螺仪带宽寄存器
            static inline constexpr uint8_t BW_MUSTSET_MASK = 0x80; ///< 陀螺仪带宽寄存器必须设置掩码

            /**
             * @brief 获取陀螺仪带宽寄存器值
             * @param odr 输出数据率设置
             * @return 带宽寄存器值
             */
            static constexpr uint8_t GetBwValue(OutputDataRate odr)
            {
                return static_cast<uint8_t>(odr) | BW_MUSTSET_MASK;
            }

            static inline constexpr uint8_t PWR_CTRL_REG = 0x11;          ///< 陀螺仪电源控制寄存器
            static inline constexpr uint8_t PWR_NORMAL_MODE = 0x00;       ///< 陀螺仪正常模式
            static inline constexpr uint8_t PWR_SUSPEND_MODE = 0x80;      ///< 陀螺仪悬挂模式
            static inline constexpr uint8_t PWR_DEEP_SUSPEND_MODE = 0x20; ///< 陀螺仪深度悬挂模式

            static inline constexpr uint8_t SOFTRESET_REG = 0x14;   ///< 陀螺仪软复位寄存器
            static inline constexpr uint8_t SOFTRESET_VALUE = 0xB6; ///< 陀螺仪软复位命令

            static inline constexpr uint8_t CTRL_REG = 0x15;           ///< 陀螺仪控制寄存器
            static inline constexpr uint8_t CTRL_DRDY_OFF_MASK = 0x00; ///< 陀螺仪数据就绪关闭
            static inline constexpr uint8_t CTRL_DRDY_ON_MASK = 0x80;  ///< 陀螺仪数据就绪开启

            static inline constexpr uint8_t INT_IO_CONF_REG = 0x16; ///< 陀螺仪GPIO中断配置寄存器
            static inline constexpr uint8_t INT_IO_MAP_REG = 0x18;  ///< 陀螺仪GPIO中断映射寄存器

            static inline constexpr uint8_t SELF_TEST_REG = 0x3C;                    ///< 陀螺仪自检寄存器
            static inline constexpr uint8_t SELF_TEST_RATE_OK_MASK = 0x01 << 0x04;   ///< 陀螺仪自检速率就绪掩码
            static inline constexpr uint8_t SELF_TEST_BIST_FAIL_MASK = 0x01 << 0x02; ///< 陀螺仪自检失败掩码
            static inline constexpr uint8_t SELF_TEST_BIST_RDY_MASK = 0x01 << 0x01;  ///< 陀螺仪自检就绪掩码
            static inline constexpr uint8_t SELF_TEST_TRIG_BIST_MASK = 0x01 << 0x00; ///< 陀螺仪触发自检掩码

            /**
             * @brief 获取带宽
             * @param odr 输出数据率枚举值
             * @return 带宽（单位：Hz）
             */
            static constexpr float GetBaudwidth(OutputDataRate odr)
            {
                switch (odr)
                {
                case OutputDataRate::ODR_2000_532_HZ:
                    return 230.0f;
                case OutputDataRate::ODR_2000_230_HZ:
                    return 116.0f;
                case OutputDataRate::ODR_1000_116_HZ:
                    return 47.0f;
                case OutputDataRate::ODR_400_47_HZ:
                    return 23.0f;
                case OutputDataRate::ODR_200_23_HZ:
                    return 12.0f;
                case OutputDataRate::ODR_100_12_HZ:
                    return 6.0f;
                case OutputDataRate::ODR_200_64_HZ:
                    return 32.0f;
                case OutputDataRate::ODR_100_32_HZ:
                    return 16.0f;
                default:
                    return 0.0f;
                }
            }

            /**
             * @brief 获取默认协方差矩阵
             * @param odr 输出数据率枚举值
             * @return 协方差矩阵，单位：(度每秒)²
             */
            static constexpr appkit::math::Matrixf<3, 3> GetDefaultCovariance(OutputDataRate odr)
            {
                const auto baudwidth = GetBaudwidth(odr);
                return appkit::math::identity_matrix<float, 3>() *
                       ((0.014f * 0.014f) * sqrtf(baudwidth)); // 角速度噪声密度为0.014度每秒/√Hz
            }
        };
    };
}

#endif // ENABLED_MODULES_BMI088