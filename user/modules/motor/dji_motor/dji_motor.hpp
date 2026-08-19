#pragma once

#include <motor.hpp>
#include <can.hpp>

#include <osal_timer.hpp>

namespace control::motor
{
    class DjiMotorBus;

    /**
     * @brief 大疆电机类
     */
    class DjiMotor final : public Motor
    {
    public:
        /**
         * @brief 大疆电机类型枚举定义
         */
        enum class Type : uint8_t
        {
            M2006,          ///< M2006
            M3508,          ///< M3508
            GM6020_VOLTAGE, ///< GM6020 电压控制
            GM6020_CURRENT, ///< GM6020 电流控制
        };

        /**
         * @brief 构造函数
         * @param bus 大疆电机总线对象
         * @param type 电机类型
         * @param motor_id 电机ID
         */
        DjiMotor(DjiMotorBus &bus, Type type, uint8_t motor_id);

        /**
         * @brief 析构函数
         */
        ~DjiMotor();

        /**
         * @brief 启用电机
         */
        void Enable();

        /**
         * @brief 禁用电机
         */
        void Disable();

        /**
         * @brief 电流控制接口
         * @param current 目标电流，单位：A
         * @return 错误码
         */
        appkit::ErrorCode CurrentControl(float current);

        /**
         * @brief 电压控制接口
         * @param voltage 目标电压，单位：V, 范围[-25,25]
         * @return 错误码
         * @note 仅适用于M6020_VOLTAGE类型电机
         */
        appkit::ErrorCode VoltageControl(float voltage);

        /**
         * @brief 力矩控制接口
         * @param torque 目标力矩，单位：Nm
         * @return 错误码
         */
        appkit::ErrorCode TorqueControl(float torque) override;

        /**
         * @brief 获取电机测量数据
         * @return 电机测量数据引用
         */
        const Measure &GetMeasure() const override;

        /**
         * @brief 获取当前电流值
         * @return 当前电流值引用
         */
        FORCE_INLINE const float &GetCurrent() const { return current_; }

    private:
        DjiMotorBus &bus_; ///< 大疆电机总线对象引用
        Type type_;        ///< 电机类型
        uint8_t motorId_;  ///< 电机ID

        bool enabled_{};      ///< 电机使能状态

        uint8_t packIndex_{}; ///< 数据包索引
        uint8_t dataIndex_{}; ///< 数据索引

        Measure measure_{};       ///< 电机测量数据
        float current_{};         ///< 当前电流值
        uint32_t circle_count_{}; ///< 电机转圈计数

        /**
         * @brief 获取力矩转换系数
         * @param type 电机类型
         * @return 力矩转换系数
         */
        static float GetTorqueScale(Type type);

        /**
         * @brief 获取电流最大值
         * @param type 电机类型
         * @return 电流最大值
         */
        static float GetCurrentMax(Type type);

        /**
         * @brief 获取电压最大值
         * @param type 电机类型
         * @return 电压最大值
         */
        static float GetVoltageMax(Type type);

        /**
         * @brief 获取电机LSB值
         * @param type 电机类型
         * @return LSB值
         */
        static int16_t GetLsb(Type type);

        /**
         * @brief 解码接收到的CAN数据帧
         * @param in_isr 是否在中断服务程序中
         * @param motor 电机对象指针
         * @param frame 接收到的CAN数据帧引用
         */
        static void DecodeData(bool in_isr, DjiMotor *motor, const appkit::CAN::ClassicPack &frame);
    };

    /**
     * @brief 大疆电机总线类
     */
    class DjiMotorBus final
    {
    private:
        /**
         * @brief 打包的数据结构体
         */
        struct PackedData final
        {
            bool in_use[4]{};                ///< 命令使用标志
            appkit::CAN::ClassicPack pack{}; ///< CAN数据包

            /**
             * @brief 构造函数
             * @param id 数据包ID
             */
            PackedData(uint32_t id)
            {
                pack.id = id;
                pack.type = appkit::CAN::PackType::STANDARD;
                pack.data_len = 8;
            }

            /**
             * @brief 设置使用标志
             * @param index 数据索引
             * @param use 是否使用
             */
            void SetInUse(uint8_t index, bool use)
            {
                if (index < 4)
                {
                    in_use[index] = use;
                    if (!use)
                    {
                        // 清除数据
                        pack.data[index * 2] = 0;
                        pack.data[index * 2 + 1] = 0;
                    }
                }
            }

            /**
             * @brief 设置数据
             * @param index 数据索引
             * @param data 数据内容
             */
            void SetData(uint8_t index, appkit::ConstRawData data)
            {
                if (index < 4 && in_use[index])
                {
                    data.CopyTo({pack.data + index * 2, 2});
                }
            }
        };

    public:
        /**
         * @brief 构造函数
         * @param can_name CAN设备名称
         * @param send_interval_ms 发送间隔时间，单位：ms
         */
        DjiMotorBus(const char *can_name, uint32_t send_interval_ms = 1);

        /**
         * @brief 注册解码函数
         * @param motor_id 电机ID
         * @param type 电机类型
         * @param callback 解码回调函数
         */
        void RegisterDecodeFunction(uint8_t motor_id, DjiMotor::Type type, appkit::CAN::Callback callback);

        /**
         * @brief 获取数据包索引
         * @param motor_id 电机ID
         * @param type 电机类型
         * @return 数据包索引
         */
        static constexpr appkit::Result<uint8_t> GetPackIndex(uint8_t motor_id, DjiMotor::Type type)
        {
            switch (type)
            {
            case DjiMotor::Type::M2006:
                [[fallthrough]];
            case DjiMotor::Type::M3508:
                if (motor_id >= 1 && motor_id <= 4)
                {
                    return appkit::Result<uint8_t>::Ok(2); // 0x200
                }
                else if (motor_id >= 5 && motor_id <= 8)
                {
                    return appkit::Result<uint8_t>::Ok(1); // 0x1FF
                }
                else
                {
                    return appkit::Result<uint8_t>::Error(appkit::ErrorCode::INVALID_ARG);
                }
            case DjiMotor::Type::GM6020_VOLTAGE:
                if (motor_id >= 1 && motor_id <= 4)
                {
                    return appkit::Result<uint8_t>::Ok(1); // 0x1FF
                }
                else if (motor_id >= 5 && motor_id <= 7)
                {
                    return appkit::Result<uint8_t>::Ok(4); // 0x2FF
                }
                else
                {
                    return appkit::Result<uint8_t>::Error(appkit::ErrorCode::INVALID_ARG);
                }
            case DjiMotor::Type::GM6020_CURRENT:
                if (motor_id >= 1 && motor_id <= 4)
                {
                    return appkit::Result<uint8_t>::Ok(0); // 0x1FE
                }
                else if (motor_id >= 5 && motor_id <= 7)
                {
                    return appkit::Result<uint8_t>::Ok(3); // 0x2FE
                }
                else
                {
                    return appkit::Result<uint8_t>::Error(appkit::ErrorCode::INVALID_ARG);
                }
            default:
                return appkit::Result<uint8_t>::Error(appkit::ErrorCode::INVALID_ARG);
            }
        }

        /**
         * @brief 获取数据索引
         * @param motor_id 电机ID
         * @return 数据索引
         */
        static constexpr uint8_t GetDataIndex(uint8_t motor_id)
        {
            return (motor_id - 1) % 4;
        }

        /**
         * @brief 获取打包的数据对象引用
         * @param pack_index 数据包索引
         * @return 打包的数据对象引用
         */
        FORCE_INLINE PackedData &GetPackedData(uint8_t pack_index)
        {
            return packedData_[pack_index];
        }

    private:
        appkit::osal::Timer::TimerHandle timerHandle_{}; ///< 定时器任务句柄

        appkit::CAN can_{};        ///< CAN总线对象
        PackedData packedData_[5]; ///< 打包的数据对象数组 {0x1FE, 0x1FF, 0x200, 0x2FE, 0x2FF}

        /**
         * @brief 发送定时器函数
         * @param bus 大疆电机总线对象指针
         */
        static void SendTimerFunc(DjiMotorBus *bus);

        /**
         * @brief 获取接收ID
         * @param motor_id 电机ID
         * @param type 电机类型
         * @return 接收ID
         */
        static uint32_t GetRecvId(uint8_t motor_id, DjiMotor::Type type)
        {
            switch (type)
            {
            case DjiMotor::Type::M2006:
                [[fallthrough]];
            case DjiMotor::Type::M3508:
                return 0x200 + motor_id;

            case DjiMotor::Type::GM6020_VOLTAGE:
                [[fallthrough]];
            case DjiMotor::Type::GM6020_CURRENT:
                return 0x204 + motor_id;

            default:
                return 0;
            }
        }
    };
}
