#include <attitude.hpp>

#include <eskf.hpp>
#include <pid.hpp>

#include <bmi088.hpp>
#include <pwm.hpp>
#include <adc.hpp>

#include <logger.hpp>

#include <attitude_angle.msg.hpp>

using namespace appkit::time_literals;

namespace application
{
    using ImuHandler = sensor::imu::BMI088; ///< IMU传感器类型别名

    static ImuHandler *imu_sensor_ptr{nullptr}; ///< BMI088 IMU传感器实例指针

    static algorithm::ESKF eskf{};     ///< 误差状态卡尔曼滤波器实例
    static appkit::PWM heater_pwm{};   ///< 加热器PWM实例
    static appkit::ADC imu_temp_adc{}; ///< IMU温度ADC监测实例

    static appkit::Topic attitude_topic{}; ///< 姿态话题

    static appkit::osal::Thread attitude_thread_{};            ///< 姿态任务线程
    static appkit::osal::Thread temperature_control_thread_{}; ///< 温度控制任务线程

    sensor::imu::Data imu_data{};              ///< IMU数据结构体
    sensor_data::AttitudeAngle attitude_msg{}; ///< 姿态消息结构体

    static void AttitudeTask();
    static void TemperatureControlTask(const ImuHandler *imu_ptr);

    void AttitudeInit()
    {
        // 初始化imu模块
        static ImuHandler imu_sensor{"spi2_accel", "spi2_gyro", "int0_gyro"};
        if (appkit::Check(imu_temp_adc.Open("adc1_ch4")) && appkit::Check(heater_pwm.Open("timer3_ch4")))
        {
            APPKIT_LOG_INFO("IMU temperature control initialized.");
        }
        else
        {
            APPKIT_LOG_ERROR("Failed to initialize IMU sensor.");
            return;
        }
        imu_sensor_ptr = &imu_sensor;

        // 初始化姿态滤波器
        const auto &imu_cov = imu_sensor.GetImuDataConvar();
        eskf.gyroNoiseCov = imu_cov.gyro_covariance * imu_cov.gyro_scale;    // 设置陀螺仪噪声协方差
        eskf.accelNoiseCov = imu_cov.accel_covariance * imu_cov.accel_scale; // 设置加速度计噪声协方差
        eskf.chiSquareThreshold = 0.584f;                                    // 设置卡方检验阈值，3自由度，显著性水平a=0.05时，卡方分布临界值约为0.352
        // 初始化话题通信
        appkit::Topic::Domain attitude_domain{"sensor"};
        attitude_topic = appkit::Topic::CreateTopic<sensor_data::AttitudeAngle>("attitude", &attitude_domain);

        // 初始化姿态任务
        temperature_control_thread_.Create(TemperatureControlTask, &imu_sensor, "imu_temp", 1024, appkit::osal::Thread::Priority::NORMAL);
        attitude_thread_.Create(AttitudeTask, "attitude", 8 * 1024, appkit::osal::Thread::Priority::HIGH);

        APPKIT_LOG_INFO("Attitude application initialized.");
    }

    static void AttitudeTask()
    {
        appkit::Topic::FifoSuber<sensor::imu::Data> imu_suber{"bmi088", 2, &sensor::imu::GetImuTopicDomain()}; ///< IMU数据订阅者

        // 初始化滤波器姿态
        {
            static constexpr appkit::math::Vectorf<3> gravity_init{0.0f, 0.0f, -1.0f};

            appkit::math::Vectorf<3> acc_init{};
            for (uint8_t i = 0; i < 100; ++i)
            {
                if (appkit::Check(imu_suber->Pop(imu_data, 100_ms)))
                {
                    if (std::abs(appkit::math::norm(imu_data.accel) - WORLD_GRAVITY) <= 0.8f)
                    {
                        acc_init += imu_data.accel;
                    }
                    else
                    {
                        i = 0;                                 // 如果测量值异常则重置计数，重新采集数据
                        acc_init = appkit::math::Vectorf<3>{}; // 重置acc_init
                        APPKIT_LOG_WARNING("Attitude task: Initial posture anomaly detected. Restarting.");
                    }
                }
            }
            acc_init /= appkit::math::norm(acc_init);
            const auto axis_rot = appkit::math::cross(acc_init, gravity_init);
            const float angle = std::acos(appkit::math::dot(acc_init, gravity_init)); // 计算初始姿态与重力方向的夹角

            const auto sin_half_angle = std::sin(angle / 2.0f);
            const auto init_q = appkit::math::Quaternionf(
                std::cos(angle / 2.0f),
                axis_rot(0, 0) * sin_half_angle,
                axis_rot(1, 0) * sin_half_angle,
                0.0f); // yaw初始为0，无法从加速度计测量中确定

            eskf.InitPosture(init_q);
        }

        for (;;)
        {
            if (appkit::Check(imu_suber->Pop(imu_data, 100_ms)))
            {
                // 判断时间差是否合理，过大则可能是数据异常，丢弃该数据
                if (imu_data.dt > 1_ms)
                {
                    APPKIT_LOG_WARNING("Attitude task: IMU data possible anomaly. dt = %f ms", appkit::DurationCast<appkit::millisecond, float>(imu_data.dt));
                    continue;
                }

                auto start_time = appkit::Clock::steady_clock->Now();

                // 姿态更新
                const auto attitude = eskf.Process(imu_data.gyro, imu_data.accel, imu_data.dt);
                const auto processing_time = appkit::Clock::steady_clock->Now() - start_time;

                // 填充姿态消息
                attitude_msg.data.timepoint = appkit::DurationCast<appkit::millisecond, float>(imu_data.timestamp.SinceEpoch());
                attitude_msg.data.w = attitude.W();
                attitude_msg.data.x = attitude.X();
                attitude_msg.data.y = attitude.Y();
                attitude_msg.data.z = attitude.Z();
                attitude.GetRPY(attitude_msg.data.roll, attitude_msg.data.pitch, attitude_msg.data.yaw);

                // 发布姿态数据
                attitude_topic.Publish(attitude_msg);

                // 日志输出
                appkit::STDIO::Printf("%6f,%6f,%6f,%6f\n",
                                      appkit::DurationCast<appkit::millisecond, float>(processing_time),
                                      attitude_msg.data.roll * 57.2958f, // 转换为度输出
                                      attitude_msg.data.pitch * 57.2958f,
                                      attitude_msg.data.yaw * 57.2958f);
                // appkit::STDIO::Printf("%6f,%6f,%6f,%6f,%6f\n",
                //                       appkit::DurationCast<appkit::millisecond, float>(processing_time),
                //                       attitude_msg.data.w, attitude_msg.data.x, attitude_msg.data.y, attitude_msg.data.z);
            }
            else
            {
                APPKIT_LOG_WARNING("Attitude task: IMU data wait timeout.");
            }
        }
    }

    static void TemperatureControlTask(const ImuHandler *imu_ptr)
    {
        constexpr float TARGET_TEMPERATURE = 50.0f; // 目标温度50摄氏度
        constexpr float PWM_FREQUENCY = 1000.0f;    // 1kHz PWM频率

        // 初始化PWM用于加热控制
        UNUSED(heater_pwm.SetFrequency(PWM_FREQUENCY));

        // 初始化PID控制器
        algorithm::PID::Param pid_param{
            .kp = 12.0f,
            .ki = 0.08f,
            .kd = 0.0f,
            .max_output = 0.0f,
            .dead_zone = 0.0f,
            .integral_limit = 10.f,
        };
        algorithm::PositionPID heat_pid{pid_param}; ///< IMU加热PID

        float output;
        for (;;)
        {
            if (const auto voltage_result = imu_temp_adc.Read();
                appkit::Check(voltage_result))
            {
                // 获取加热电阻输入电压
                const auto voltage = voltage_result.GetValue();

                // 根据电压计算温度 (以1v输入电压为基准更新PID)
                output = heat_pid.Update(TARGET_TEMPERATURE, imu_ptr->GetTemperature(), 128_ms) / (voltage * voltage);
                output = std::clamp(output, 0.0f, 1.0f); // 限制输出在0.0到1.0之间
            }
            else
            {
                output = 0.0f;
                APPKIT_LOG_WARNING("Temperature control task: Failed to read IMU temperature.");
            }

            // 设置PWM占空比进行加热控制
            // if (!appkit::Check(heater_pwm.SetDutyCycle(output)))
            // {
            //     APPKIT_LOG_WARNING("Temp control task: Failed to set heater PWM duty cycle.");
            // }

            appkit::osal::this_thread::SleepFor(128_ms);
        }
    }
} // namespace application
