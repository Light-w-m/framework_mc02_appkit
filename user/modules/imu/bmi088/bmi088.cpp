#ifdef ENABLED_MODULES_BMI088

#include <bmi088.hpp>

#include <common_rw.hpp>
#include <logger.hpp>

using namespace appkit::time_literals;

namespace sensor::imu
{
    static constexpr float DEG2RAD = std::numbers::pi_v<float> / 180.0f; ///< 度转弧度

    constexpr appkit::Duration BMI088::BMI088_SHORT_DELAY_TIME = 1_ms;       ///< 短延时
    constexpr appkit::Duration BMI088::BMI088_LONG_DELAY_TIME = 80_ms;       ///< 长延时
    constexpr appkit::Duration BMI088::BMI088_COM_WAIT_SENSOR_TIME = 150_ms; ///< 等待传感器时间

    FORCE_INLINE void BMI088::Delay(const appkit::Duration &duration)
    {
        using namespace appkit;
        osal::this_thread::SleepFor(duration);
    }

    appkit::ErrorCode BMI088::WriteReg(appkit::SPI &spi, uint8_t reg, uint8_t data)
    {
        reg &= 0x7F; // 写寄存器，最高位为0

        const uint8_t tx_buf[2] = {reg, data};
        appkit::ConstRawData tx_data{tx_buf};

        return spi.Write(tx_data, readOp_);
    }

    appkit::ErrorCode BMI088::AccelRead(uint8_t reg, uint32_t length)
    {
        if (length == 0)
        {
            return appkit::ErrorCode::INVALID_ARG;
        }

        uint8_t reg_buf[8] = {0};
        reg_buf[0] = reg | 0x80; // 读寄存器，最高位为1

        appkit::ConstRawData tx_data{reg_buf, length + 2};
        appkit::RawData rx_data{accelRawData_.raw, length + 2};

        const auto ret = accSpi_.ReadAndWrite(tx_data, rx_data, readOp_);

        return ret;
    }

    appkit::ErrorCode BMI088::GyroRead(uint8_t reg, uint32_t length)
    {
        if (length == 0)
        {
            return appkit::ErrorCode::INVALID_ARG;
        }

        uint8_t reg_buf[7] = {0};
        reg_buf[0] = reg | 0x80; // 读寄存器，最高位为1

        appkit::ConstRawData tx_data{reg_buf, length + 1};
        appkit::RawData rx_data{gyroRawData_.raw, length + 1};

        const auto ret = gyroSpi_.ReadAndWrite(tx_data, rx_data, readOp_);

        return ret;
    }

    bool BMI088::Init()
    {
        status_ = Status::NO_ERROR;

        Delay(BMI088_SHORT_DELAY_TIME);

        // fake write 切换到SPI模式
        Delay(BMI088_COM_WAIT_SENSOR_TIME);
        AccelRead(BMI088Def::Accel::CHIP_ID_REG, 1);
        Delay(BMI088_SHORT_DELAY_TIME);
        AccelRead(BMI088Def::Accel::CHIP_ID_REG, 1);
        Delay(BMI088_SHORT_DELAY_TIME);

        // 重启传感器
        WriteReg(gyroSpi_, BMI088Def::Gyro::SOFTRESET_REG, BMI088Def::Gyro::SOFTRESET_VALUE);
        WriteReg(accSpi_, BMI088Def::Accel::SOFTRESET_REG, BMI088Def::Accel::SOFTRESET_VALUE);
        Delay(BMI088_COM_WAIT_SENSOR_TIME * 2);

        // 检查id
        AccelRead(BMI088Def::Accel::CHIP_ID_REG, 1);
        GyroRead(BMI088Def::Gyro::CHIP_ID_REG, 1);
        AccelRead(BMI088Def::Accel::CHIP_ID_REG, 1);
        GyroRead(BMI088Def::Gyro::CHIP_ID_REG, 1);

        if (accelRawData_.data.chip_id != BMI088Def::Accel::CHIP_ID_VALUE)
        {
            status_ = Status::NO_SENSOR;
            return false;
        }

        if (gyroRawData_.data.chip_id != BMI088Def::Gyro::CHIP_ID_VALUE)
        {
            status_ = Status::NO_SENSOR;
            return false;
        }

        // 初始化accel
        // 配置电源控制寄存器
        if (auto ret = WriteReg(accSpi_, BMI088Def::Accel::PWR_CTRL_REG, BMI088Def::Accel::PWR_ENABLE_ACC_ON);
            !appkit::Check(ret))
        {
            status_ = Status::ACC_PWR_CTRL_ERROR;
            return false;
        }
        Delay(BMI088_SHORT_DELAY_TIME);

        // 配置电源配置寄存器
        if (auto ret = WriteReg(accSpi_, BMI088Def::Accel::PWR_CONF_REG, BMI088Def::Accel::PWR_ACTIVE_MODE);
            !appkit::Check(ret))
        {
            status_ = Status::ACC_PWR_CONF_ERROR;
            return false;
        }
        Delay(BMI088_SHORT_DELAY_TIME);

        // 配置加速度计配置寄存器
        if (auto ret = WriteReg(
                accSpi_,
                BMI088Def::Accel::ACC_CONF_REG,
                BMI088Def::Accel::GetAccConfValue(
                    BMI088Def::Accel::OverSampling::BMI088_ACCEL_OVERSAMPLING,
                    BMI088Def::Accel::OutputDataRate::BMI088_ACCEL_ODR));
            !appkit::Check(ret))
        {
            status_ = Status::ACC_CONF_ERROR;
            return false;
        }
        Delay(BMI088_SHORT_DELAY_TIME);

        // 配置加速度计量程寄存器
        if (auto ret = WriteReg(
                accSpi_,
                BMI088Def::Accel::ACC_RANGE_REG,
                BMI088Def::Accel::GetAccRangeValue(BMI088Def::Accel::Range::BMI088_ACCEL_RANGE));
            !appkit::Check(ret))
        {
            status_ = Status::ACC_RANGE_ERROR;
            return false;
        }
        Delay(BMI088_SHORT_DELAY_TIME);

        // 初始化gyro
        // 配置陀螺仪量程寄存器
        if (auto ret = WriteReg(
                gyroSpi_,
                BMI088Def::Gyro::RANGE_REG,
                BMI088Def::Gyro::GetRangeValue(BMI088Def::Gyro::Range::BMI088_GYRO_RANGE));
            !appkit::Check(ret))
        {
            status_ = Status::GYRO_RANGE_ERROR;
            return false;
        }
        Delay(BMI088_SHORT_DELAY_TIME);

        // 配置陀螺仪带宽寄存器
        if (auto ret = WriteReg(
                gyroSpi_,
                BMI088Def::Gyro::BW_REG,
                BMI088Def::Gyro::GetBwValue(BMI088Def::Gyro::OutputDataRate::BMI088_GYRO_ODR));
            !appkit::Check(ret))
        {
            status_ = Status::GYRO_BANDWIDTH_ERROR;
            return false;
        }
        Delay(BMI088_SHORT_DELAY_TIME);

        // 配置陀螺仪低功耗模式寄存器
        if (auto ret = WriteReg(
                gyroSpi_,
                BMI088Def::Gyro::PWR_CTRL_REG,
                BMI088Def::Gyro::PWR_NORMAL_MODE);
            !appkit::Check(ret))
        {
            status_ = Status::GYRO_LPM1_ERROR;
            return false;
        }
        Delay(BMI088_SHORT_DELAY_TIME);

        // 配置陀螺仪控制寄存器
        if (auto ret = WriteReg(
                gyroSpi_,
                BMI088Def::Gyro::CTRL_REG,
                BMI088Def::Gyro::CTRL_DRDY_ON_MASK);
            !appkit::Check(ret))
        {
            status_ = Status::GYRO_CTRL_ERROR;
            return false;
        }
        Delay(BMI088_SHORT_DELAY_TIME);

        // 配置陀螺仪中断引脚配置寄存器
        if (auto ret = WriteReg(
                gyroSpi_,
                BMI088Def::Gyro::INT_IO_CONF_REG,
#if BMI088_GYRO_INT3_ENABLE == 1
                BMI088Def::Gyro::GpioInt::GetConfValue(
                    BMI088Def::Gyro::GpioIntPin::INT3,
                    BMI088Def::Gyro::GpioIntMode::BMI088_GYRO_INT3_MODE,
                    BMI088Def::Gyro::GpioIntLevel::BMI088_GYRO_INT3_LEVEL) |
#endif
#if BMI088_GYRO_INT4_ENABLE == 1
                    BMI088Def::Gyro::GpioInt::GetConfValue(
                        BMI088Def::Gyro::GpioIntPin::INT4,
                        BMI088Def::Gyro::GpioIntMode::BMI088_GYRO_INT4_MODE,
                        BMI088Def::Gyro::GpioIntLevel::BMI088_GYRO_INT4_LEVEL) |
#endif
                    0);
            !appkit::Check(ret))
        {
            status_ = Status::GYRO_INT3_INT4_IO_CONF_ERROR;
            return false;
        }
        Delay(BMI088_SHORT_DELAY_TIME);

        // 配置陀螺仪中断引脚使能寄存器
        if (auto ret = WriteReg(
                gyroSpi_,
                BMI088Def::Gyro::INT_IO_MAP_REG,
                BMI088Def::Gyro::GpioInt::GetEnableValue(
#if BMI088_GYRO_INT3_ENABLE == 1
                    true,
#else
                    false,
#endif
#if BMI088_GYRO_INT4_ENABLE == 1
                    true
#else
                    false
#endif
                    ));
            !appkit::Check(ret))
        {
            status_ = Status::GYRO_INT3_INT4_IO_MAP_ERROR;
            return false;
        }
        Delay(BMI088_SHORT_DELAY_TIME);

        // 初始化完成
        return true;
    }

    void BMI088::ReadThread()
    {
        appkit::TimePoint read_time{};

        for (;;)
        {
            // 获取中断时间点
            if (appkit::Check(readQueue_.Pop(read_time, BMI088_COM_WAIT_SENSOR_TIME)))
            {
                // 读取数据
                // 读取陀螺仪数据
                GyroRead(BMI088Def::Gyro::DATA_X_L_REG, 6);
                imuData_.gyro = appkit::math::Vectorf<3>{
                                    BMI088Def::Gyro::GetGyroData(gyroRawData_.data.axis[0], BMI088Def::Gyro::Range::BMI088_GYRO_RANGE),
                                    BMI088Def::Gyro::GetGyroData(gyroRawData_.data.axis[1], BMI088Def::Gyro::Range::BMI088_GYRO_RANGE),
                                    BMI088Def::Gyro::GetGyroData(gyroRawData_.data.axis[2], BMI088Def::Gyro::Range::BMI088_GYRO_RANGE)} *
                                    DEG2RAD -
                                gyroBias_;

                // 校准陀螺仪
                if (inCalibration_)
                {
                    gyroCaliData_ += imuData_.gyro;
                    calibCount_++;
                }

                // 读取加速度计数据
                AccelRead(BMI088Def::Accel::DATA_X_L_REG, 6);
                imuData_.accel = appkit::math::Vectorf<3>{
                                     BMI088Def::Accel::GetAccData(accelRawData_.data.axis[0], BMI088Def::Accel::Range::BMI088_ACCEL_RANGE),
                                     BMI088Def::Accel::GetAccData(accelRawData_.data.axis[1], BMI088Def::Accel::Range::BMI088_ACCEL_RANGE),
                                     BMI088Def::Accel::GetAccData(accelRawData_.data.axis[2], BMI088Def::Accel::Range::BMI088_ACCEL_RANGE)} *
                                 (-(WORLD_GRAVITY));

                // 读取温度数据
                AccelRead(BMI088Def::Accel::TEMP_H_REG, 2);
                temperature_ = BMI088Def::Accel::GetTemperature(
                    accelRawData_.data.temp.temp_h,
                    accelRawData_.data.temp.temp_l);

                // 更新数据时间戳
                imuData_.dt = read_time - imuData_.timestamp;
                imuData_.timestamp = read_time;

                // 发布话题
                topic_.Publish(imuData_);
            }
            else
            {
                APPKIT_LOG_WARNING("BMI088 read timeout");
            }
        }
    }

    void BMI088::Calibration()
    {
        // 重置校准变量
        calibCount_ = 0;
        gyroCaliData_ = appkit::math::Vectorf<3>{};
        gyroBias_ = appkit::math::Vectorf<3>{};

        APPKIT_LOG_INFO("BMI088 calibration started");
        inCalibration_ = true;

        // 等待校准完成
        appkit::osal::this_thread::SleepFor(appkit::Duration::From<appkit::millisecond>(BMI088_CALIBRATION_TIME));

        inCalibration_ = false;

        // 计算陀螺仪偏置
        if (calibCount_ > 0)
        {
            gyroBias_ = gyroCaliData_ / static_cast<float>(calibCount_);
        }
        else
        {
            APPKIT_LOG_WARNING("BMI088 calibration count is zero");
        }

        APPKIT_LOG_DEBUG("BMI088 gyro bias: x=%.4f, y=%.4f, z=%.4f", gyroBias_(0, 0), gyroBias_(1, 0), gyroBias_(2, 0));
        APPKIT_LOG_INFO("BMI088 calibration completed");
    }

    BMI088::BMI088(const char *acc_spi_name, const char *gyro_spi_name, const char *gyro_int_name)
        : readQueue_(4)
    {
        // 初始化默认协方差矩阵
        imuDataConvar_.accel_covariance = BMI088Def::Accel::GetDefaultCovariance(
                                              BMI088Def::Accel::OutputDataRate::BMI088_ACCEL_ODR,
                                              BMI088Def::Accel::OverSampling::BMI088_ACCEL_OVERSAMPLING) *
                                          (WORLD_GRAVITY * WORLD_GRAVITY);
        imuDataConvar_.gyro_covariance = BMI088Def::Gyro::GetDefaultCovariance(
                                             BMI088Def::Gyro::OutputDataRate::BMI088_GYRO_ODR) *
                                         (DEG2RAD * DEG2RAD);

        // 初始化硬件接口
        APPKIT_RAISE_IF_NOT(appkit::Check(accSpi_.Open(acc_spi_name)), "Failed to open BMI088 accel SPI device");
        APPKIT_RAISE_IF_NOT(appkit::Check(gyroSpi_.Open(gyro_spi_name)), "Failed to open BMI088 gyro SPI device");
        APPKIT_RAISE_IF_NOT(appkit::Check(gyroInt_.Open(gyro_int_name)), "Failed to open BMI088 gyro interrupt device");

        // 初始化读写操作模式
        readOp_ = appkit::ReadOperation(opSem_, BMI088_LONG_DELAY_TIME);
        // 初始化话题
        topic_ = appkit::Topic::CreateTopic<Data>("bmi088", &GetImuTopicDomain());

        // 初始化外部中断
        gyroInt_.SetInterrupt(false);
        gyroInt_.RegisterCallback(appkit::GPIO::Callback::Create(
            [this](bool in_isr)
            { readQueue_.Push(appkit::Clock::steady_clock->Now()); }));
        gyroInt_.SetInterrupt(true);

        // 初始化传感器
        while (!Init())
        {
            APPKIT_LOG_ERROR("BMI088 initialization failed, retrying...");
            Delay(BMI088_COM_WAIT_SENSOR_TIME);
        }

        // 初始化读取线程
        readThread_.Create(
            [this]()
            { ReadThread(); },
            "BMI088_Read",
            BMI088_THREAD_DEPTH,
            appkit::osal::Thread::Priority::HIGH);

#if BMI088_CALIBRATION_ON_STARTUP == 1
        // 自动校准
        Calibration();
#endif

        // 初始化完成
        APPKIT_LOG_INFO("BMI088 initialized successfully");
    }
} // namespace sensor::imu

#endif // ENABLED_MODULES_BMI088