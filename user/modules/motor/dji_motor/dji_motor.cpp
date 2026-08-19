#include <dji_motor.hpp>

#include <cycle.hpp>
#include <logger.hpp>

namespace control::motor
{
    DjiMotor::DjiMotor(DjiMotorBus &bus, Type type, uint8_t motor_id)
        : bus_(bus), type_(type), motorId_(motor_id)
    {
        // 获取数据包索引和数据索引
        const auto pack_index_res = bus_.GetPackIndex(motor_id, type);
        APPKIT_RAISE_IF_NOT(pack_index_res, "Invalid motor ID");
        packIndex_ = pack_index_res.GetValue();

        // 获取数据索引
        dataIndex_ = bus_.GetDataIndex(motor_id);

        // 设置电机使用标志
        bool has_used = bus_.GetPackedData(packIndex_).in_use[dataIndex_];
        if (type == Type::GM6020_CURRENT || type == Type::GM6020_VOLTAGE)
        {
            // GM6020电机有两种控制模式，需要检查两个数据包
            const Type other_type = (type == Type::GM6020_CURRENT) ? Type::GM6020_VOLTAGE : Type::GM6020_CURRENT;
            has_used = has_used || bus_.GetPackedData(bus_.GetPackIndex(motor_id, other_type).GetValue()).in_use[dataIndex_];
        }
        APPKIT_RAISE_IF(has_used, "Motor ID is already in use");

        bus_.GetPackedData(packIndex_).SetInUse(dataIndex_, true);

        // 注册数据解码函数
        bus_.RegisterDecodeFunction(motor_id, type, appkit::CAN::Callback::Create(DecodeData, this));
    }

    DjiMotor::~DjiMotor()
    {
        // 清除电机使用标志
        bus_.GetPackedData(packIndex_).SetInUse(dataIndex_, false);
    }

    void DjiMotor::Enable()
    {
        enabled_ = true;
    }

    void DjiMotor::Disable()
    {
        static constexpr float ZERO_TORQUE = 0.0f;

        enabled_ = false;
        bus_.GetPackedData(packIndex_).SetData(dataIndex_, appkit::ConstRawData(ZERO_TORQUE)); // 发送零力矩命令以停止电机
    }

    appkit::ErrorCode DjiMotor::CurrentControl(float current)
    {
        if (type_ == Type::GM6020_VOLTAGE) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Current control is not supported for GM6020_VOLTAGE type");
            return appkit::ErrorCode::NOT_SUPPORTED;
        }

        // 检查电机是否启用
        if (!enabled_)
        {
            return appkit::ErrorCode::FAILED;
        }

        // 计算发送值
        int16_t send_current = static_cast<int16_t>(std::clamp(current / GetCurrentMax(type_), -1.0f, 1.0f) * GetLsb(type_));

        // 打包数据
        bus_.GetPackedData(packIndex_).SetData(dataIndex_, appkit::ConstRawData(send_current));

        return appkit::ErrorCode::OK;
    }

    appkit::ErrorCode DjiMotor::VoltageControl(float voltage)
    {
        if (type_ != Type::GM6020_VOLTAGE) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Voltage control is only supported for GM6020_VOLTAGE type");
            return appkit::ErrorCode::NOT_SUPPORTED;
        }

        // 检查电机是否启用
        if (!enabled_)
        {
            return appkit::ErrorCode::FAILED;
        }

        // 计算发送值
        int16_t send_voltage = static_cast<int16_t>(std::clamp(voltage / GetVoltageMax(type_), -1.0f, 1.0f) * GetLsb(type_));

        // 打包数据
        bus_.GetPackedData(packIndex_).SetData(dataIndex_, appkit::ConstRawData(send_voltage));

        return appkit::ErrorCode::OK;
    }

    appkit::ErrorCode DjiMotor::TorqueControl(float torque)
    {
        // 计算目标电流
        float target_current = torque / GetTorqueScale(type_);

        return CurrentControl(target_current);
    }

    const DjiMotor::Measure &DjiMotor::GetMeasure() const
    {
        return measure_;
    }

    float DjiMotor::GetTorqueScale(DjiMotor::Type type)
    {
        switch (type)
        {
        case Type::M2006:
            return 0.006923076923076923;
        case Type::M3508:
            return 0.015789473684210526;
        case Type::GM6020_CURRENT:
            return 0.741f;
        default:
            return std::numeric_limits<float>::infinity();
        }
    }

    float DjiMotor::GetCurrentMax(DjiMotor::Type type)
    {
        switch (type)
        {
        case Type::M2006:
            return 10.0f;
        case Type::M3508:
            return 20.0f;
        case Type::GM6020_CURRENT:
            return 3.0f;
        default:
            return 0.0f;
        }
    }

    float DjiMotor::GetVoltageMax(DjiMotor::Type type)
    {
        switch (type)
        {
        case Type::GM6020_VOLTAGE:
            return 25.0f;
        default:
            return 0.0f;
        }
    }

    int16_t DjiMotor::GetLsb(DjiMotor::Type type)
    {
        switch (type)
        {
        case Type::M2006:
            return 10000;
        case Type::M3508:
            return 16384;
        case Type::GM6020_VOLTAGE:
            return 25000;
        case Type::GM6020_CURRENT:
            return 16384;
        default:
            return 0;
        }
    }

    void DjiMotor::DecodeData(bool in_isr, DjiMotor *motor, const appkit::CAN::ClassicPack &frame)
    {
        static constexpr float RPM2RAD = 2.0f * std::numbers::pi_v<float> / 60.0f;
        const auto &data = frame.data;

        float last_abs_pos = motor->measure_.abs_pos;
        float current_angle = ((uint16_t)data[0] << 8 | data[1]) / 8191.0f * (2.0f * std::numbers::pi_v<float>);

        // 计算绝对位置，处理转圈计数
        if (current_angle - last_abs_pos > std::numbers::pi_v<float>)
        {
            motor->circle_count_--;
        }
        else if (current_angle - last_abs_pos < -std::numbers::pi_v<float>)
        {
            motor->circle_count_++;
        }

        motor->measure_.abs_pos = current_angle + motor->circle_count_ * (2.0f * std::numbers::pi_v<float>);
        motor->measure_.single_pos = appkit::math::CycleValue(current_angle).GetCycleAngle(0.0f, 2.0f * std::numbers::pi_v<float>);

        // 更新速度、力矩和温度数据
        motor->measure_.speed = ((uint16_t)data[2] << 8 | data[3]) * RPM2RAD;
        if (motor->type_ == Type::M2006)
        {
            // M2006反馈为力矩值
            motor->measure_.torque = (((int16_t)data[4] << 8) | data[5]);
        }
        else
        {
            motor->current_ = ((int16_t)data[4] << 8) | data[5];
            motor->measure_.torque = motor->current_ * GetTorqueScale(motor->type_);
        }
        motor->measure_.temperature = static_cast<float>(data[6]);
    }

    DjiMotorBus::DjiMotorBus(const char *can_name, uint32_t send_interval_ms)
        : packedData_{
              PackedData(0x1FE),
              PackedData(0x1FF),
              PackedData(0x200),
              PackedData(0x2FE),
              PackedData(0x2FF),
          }
    {
        // 打开CAN设备
        APPKIT_RAISE_IF_NOT(appkit::Check(can_.Open(can_name)), "Failed to open CAN device");

        // 启动发送定时器
        timerHandle_ = appkit::osal::Timer::Create(SendTimerFunc, this, send_interval_ms);
    }

    void DjiMotorBus::RegisterDecodeFunction(uint8_t motor_id, DjiMotor::Type type, appkit::CAN::Callback callback)
    {
        const auto recv_id = GetRecvId(motor_id, type);
        const auto ret = can_.Register(callback, appkit::CAN::PackType::STANDARD, appkit::CAN::FilterMode::EQUAL, recv_id);
        APPKIT_RAISE_IF_NOT(appkit::Check(ret), "Failed to register CAN receive callback");
    }

    void DjiMotorBus::SendTimerFunc(DjiMotorBus *bus)
    {
        for (auto i = 0; i < 5; ++i)
        {
            const auto &data = bus->packedData_[i];
            if (data.in_use[0] || data.in_use[1] || data.in_use[2] || data.in_use[3])
            {
                const auto ret = bus->can_.AddMessage(data.pack);
                if (!appkit::Check(ret)) [[unlikely]]
                {
                    APPKIT_LOG_ERROR("Failed to send CAN message with ID 0x%X", data.pack.id);
                }
            }
        }
    }
} // namespace control::motor