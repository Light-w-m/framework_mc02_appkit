#pragma once

#include <common_mem.hpp>
#include <osal_thread.hpp>

#include <can.hpp>
#include <motor.hpp>

#include <cycle.hpp>
#include <logger.hpp>

namespace control::motor
{
    /**
     * @brief CAN句柄类型枚举定义
     */
    enum class DmHandleType : uint8_t
    {
        CLASSIC_CAN, ///< 经典CAN
        FD_CAN       ///< CAN FD
    };

    /**
     * @brief 达妙电机类
     * @tparam CanType CAN类型枚举值
     */
    template <DmHandleType CanType>
    class DMMotor final : public Motor
    {
    private:
        // 根据CAN类型选择对应的硬件接口和数据包类型
        using HwType = std::conditional_t<
            (CanType == DmHandleType::CLASSIC_CAN),
            appkit::CAN,
            appkit::FDCAN>;

        using HwPackType = std::conditional_t<
            (CanType == DmHandleType::CLASSIC_CAN),
            appkit::CAN::ClassicPack,
            appkit::FDCAN::FDPack>;

        /**
         * @brief 电机寄存器枚举定义
         */
        enum class Register : uint8_t
        {
            UV_Value = 0,   ///< 低压保护值 RW (10.0,fmax] float
            KT_Value = 1,   ///< 扭矩系数 RW [0.0,fmax] float
            OT_Value = 2,   ///< 过温保护值 RW [80.0,200) float
            OC_Value = 3,   ///< 过流保护值 RW (0.0,1.0) float
            ACC = 4,        ///< 加速度 RW (0.0,fmax) float
            DEC = 5,        ///< 减速度 RW [-fmax,0.0) float
            MAX_SPD = 6,    ///< 最大速度 RW (0.0,fmax] float
            MST_ID = 7,     ///< 反馈 ID RW [0,0x7FF] uint32
            ESC_ID = 8,     ///< 接收 ID RW [0,0x7FF] uint32
            TIMEOUT = 9,    ///< 超时警报时间 RW [0,2^32-1] uint32
            CTRL_MODE = 10, ///< 控制模式 RW [0,4] uint32
            Damp = 11,      ///< 电机粘滞系数 RO / float
            Inertia = 12,   ///< 电机转动惯量 RO / float
            hw_ver = 13,    ///< 保留 RO / uint32
            sw_ver = 14,    ///< 软件版本号 RO / uint32
            SN = 15,        ///< 保留 RO / uint32
            NPP = 16,       ///< 电机极对数 RO / uint32
            Rs = 17,        ///< 电机相电阻 RO / float
            LS = 18,        ///< 电机相电感 RO / float
            Flux = 19,      ///< 电机磁链值 RO / float
            Gr = 20,        ///< 齿轮减速比 RO / float
            PMAX = 21,      ///< 位置映射范围 RW (0.0,fmax] float
            VMAX = 22,      ///< 速度映射范围 RW (0.0,fmax] float
            TMAX = 23,      ///< 扭矩映射范围 RW (0.0,fmax] float
            I_BW = 24,      ///< 电流环控制带宽 RW [100.0,1.0e4] float
            KP_ASR = 25,    ///< 速度环 Kp RW [0.0,fmax] floa
            KI_ASR = 26,    ///< 速度环 Ki RW [0.0,fmax] float
            KP_APR = 27,    ///< 位置环 Kp RW [0.0,fmax] float
            KI_APR = 28,    ///< 位置环 Ki RW [0.0,fmax] float
            OV_Value = 29,  ///< 过压保护值 RW TBD float
            GREF = 30,      ///< 齿轮力矩效率 RW (0.0,1.0] float
            Deta = 31,      ///< 速度环阻尼系数 RW [1.0,30.0] float
            V_BW = 32,      ///< 速度环滤波带宽 RW (0.0,500.0) float
            IQ_c1 = 33,     ///< 电流环增强系数 RW [100.0, 1.0e4] float
            VL_c1 = 34,     ///< 速度环增强系数 RW (0.0,1.0e4] float
            can_br = 35,    ///< CAN 波特率代码 RW [0,4] uint32
            sub_ver = 36,   ///< 子版本号 RO / uint32
            u_off = 50,     ///< u相偏置 RO / float
            v_off = 51,     ///< v相偏置 RO / float
            k1 = 52,        ///< 补偿因子1 RO / float
            k2 = 53,        ///< 补偿因子2 RO / float
            m_off = 54,     ///< 角度偏移 RO / float
            dir = 55,       ///< 方向 RO / float
            p_m = 80,       ///< 电机当前位置 RO / float
            xout = 81,      ///< 输出轴位置 RO / float
        };

        /**
         * @brief 寄存器操作枚举定义
         */
        enum class RegOperation : uint8_t
        {
            READ = 0x33,  ///< 读寄存器
            WRITE = 0x55, ///< 写寄存器
            SAVE = 0xAA   ///< 保存参数
        };

#pragma pack(push, 1)
        /**
         * @brief 电机寄存器帧结构体模板
         * @tparam T 寄存器值类型
         */
        template <typename T>
        struct RegFrame final
        {
            uint16_t can_id;  ///< CAN ID
            uint8_t cmd;      ///< 命令字
            uint8_t reg_addr; ///< 寄存器地址
            T value;          ///< 寄存器值

            /**
             * @brief 构造函数
             * @param id CAN ID
             * @param op 寄存器操作
             * @param reg 寄存器
             */
            RegFrame(uint16_t id, RegOperation op, Register reg, T val)
                : can_id(id), cmd(static_cast<uint8_t>(op)), reg_addr(static_cast<uint8_t>(reg)), value(val)
            {
            }
        };

        /**
         * @brief 电机寄存器帧结构体模板特化（无寄存器值）
         */
        struct RegFrameNoVal final
        {
            uint16_t can_id;  ///< CAN ID
            uint8_t cmd;      ///< 命令字
            uint8_t reg_addr; ///< 寄存器地址

            /**
             * @brief 构造函数
             * @param id CAN ID
             * @param op 寄存器操作
             * @param reg 寄存器
             */
            RegFrameNoVal(uint16_t id, RegOperation op, Register reg)
                : can_id(id), cmd(static_cast<uint8_t>(op)), reg_addr(static_cast<uint8_t>(reg))
            {
            }
        };

        struct FeedbackFrame final
        {
            uint8_t error_flag : 4; ///< 错误标志位
            uint8_t id : 4;         ///< 控制器的 ID，取 CAN_ID 的低 8 位
            uint16_t position;      ///< 位置值
            uint16_t speed : 12;    ///< 速度值
            uint16_t torque : 12;   ///< 扭矩值
            uint8_t mos_temp;       ///< 驱动上 MOS 的平均温度，单位℃
            uint8_t rotor_temp;     ///< 电机内部线圈的平均温度，单位℃
        };
#pragma pack(pop)

        static constexpr uint32_t BOARDCAST_ID = 0x7FF; ///< 广播 CAN ID

        static constexpr float KP_MIN = 0.0f;   ///< 位置环 Kp 最小值
        static constexpr float KP_MAX = 500.0f; ///< 位置环 Kp 最大值
        static constexpr float KD_MIN = 0.0f;   ///< 位置环 Kd 最小值
        static constexpr float KD_MAX = 5.0f;   ///< 位置环 Kd 最大值

        /**
         * @brief 将无符号整数转换为浮点数
         * @param val 无符号整数值
         * @param min 最小浮点数值
         * @param max 最大浮点数值
         * @param bits 位数
         * @return 浮点数值
         */
        static constexpr float Uint2Float(uint32_t val, float min, float max, uint8_t bits)
        {
            float span = max - min;
            float offset = min;
            return ((float)val) * span / ((float)((1 << bits) - 1)) + offset;
        }

        /**
         * @brief 将浮点数转换为无符号整数
         * @param val 浮点数值
         * @param min 最小浮点数值
         * @param max 最大浮点数值
         * @param bits 位数
         * @return 无符号整数值
         */
        static constexpr uint32_t Float2Uint(float val, float min, float max, uint8_t bits)
        {
            float span = max - min;
            float offset = min;
            return static_cast<uint32_t>(((val - offset) * ((1 << bits) - 1)) / span);
        }

        /**
         * @brief 发送寄存器帧
         * @param frame 寄存器帧引用
         * @return 错误码
         */
        template <typename T>
            requires std::disjunction_v<std::is_same<T, RegFrameNoVal>, std::is_same<T, RegFrame<uint32_t>>, std::is_same<T, RegFrame<float>>>
        appkit::ErrorCode SendRegFrame(const T &frame)
        {
            HwPackType pack{};
            pack.id = BOARDCAST_ID;
            pack.type = HwType::PackType::STANDARD;
            pack.data_len = sizeof(T);
            appkit::Memory::Copy(pack.data, &frame, sizeof(T));

            return hw_.AddMessage(pack);
        }

    public:
        /**
         * @brief 电机型号枚举定义
         */
        enum class Type : uint8_t
        {
            // 关节电机
            J3507,
            J4310,
            J4310P,
            J4340,
            J4340P,
            J6006,
            J8006,
            J8009,
            J8009P,
            J10010,
            J10010L,
            J6248P,

            // 中空电机
            G6220,

            // 轮毂电机
            H3510,
            H6215,
            H6530
        };

        /**
         * @brief 电机控制模式枚举定义
         */
        enum class ControlMode : uint16_t
        {
            MIT = 0x000,       ///< MIT控制模式
            POS_VEL = 0x100,   ///< 位置速度控制模式
            VEL = 0x200,       ///< 速度控制模式
            POS_FORCE = 0x300, ///< 位置力矩控制模式
        };

        /**
         * @brief 构造函数
         * @param hw CAN硬件接口对象引用
         * @param can_id CAN ID
         * @param motor_type 电机类型
         * @param control_mode 控制模式
         */
        DMMotor(const char *can_name, uint32_t can_id, uint32_t feedback_id, Type motor_type, ControlMode control_mode)
            : hw_(), canId_(can_id), feedbackId_(feedback_id), motorType_(motor_type), controlMode_(control_mode)
        {
            using namespace appkit::time_literals;

            APPKIT_RAISE_IF_NOT(can_name != nullptr, "CAN name must not be null");

            // 打开CAN设备
            APPKIT_RAISE_IF_NOT(appkit::Check(hw_.Open(can_name)), "Failed to open CAN device");

            // 注册接收回调函数
            hw_.Register(HwType::Callback::Create(DecodeData, this), HwType::PackType::STANDARD, HwType::FilterMode::EQUAL, feedbackId_);
            hw_.Register(HwType::Callback::Create(DecodeData, this), HwType::PackType::STANDARD, HwType::FilterMode::EQUAL, BOARDCAST_ID);

            // 读取电机参数
            APPKIT_RAISE_IF_NOT(appkit::Check(SendRegFrame({canId_, RegOperation::READ, Register::PMAX})),
                                "Failed to read motor PMAX");
            APPKIT_RAISE_IF_NOT(appkit::Check(SendRegFrame({canId_, RegOperation::READ, Register::VMAX})),
                                "Failed to read motor VMAX");
            APPKIT_RAISE_IF_NOT(appkit::Check(SendRegFrame({canId_, RegOperation::READ, Register::TMAX})),
                                "Failed to read motor TMAX");

            // 等待接收到参数反馈
            while (pMax_ == 0 || vMax_ == 0 || tMax_ == 0)
            {
                appkit::osal::this_thread::SleepFor(100_ms);
            }

            // 设置电机控制模式
            APPKIT_RAISE_IF_NOT(appkit::Check(SetControlMode(controlMode_)), "Failed to set motor control mode");
        }

        /**
         * @brief 设置电机控制模式
         * @param mode 控制模式
         * @return 错误码
         */
        appkit::ErrorCode SetControlMode(ControlMode mode)
        {
            RegFrame<uint32_t> reg_value{canId_, RegOperation::Write, Register::CTRL_MODE, 0};
            switch (mode)
            {
            case ControlMode::MIT:
                reg_value.value = 1;
                break;
            case ControlMode::POS_VEL:
                reg_value.value = 2;
                break;
            case ControlMode::VEL:
                reg_value.value = 3;
                break;
            case ControlMode::POS_FORCE:
                reg_value.value = 4;
                break;
            default:
                return appkit::ErrorCode::INVALID_ARG;
            }

            const auto ret = SendRegFrame(reg_value);
            if (appkit::Check(ret)) [[likely]]
            {
                controlMode_ = mode;
            }
            else
            {
                APPKIT_LOG_ERROR("Failed to send control mode frame to motor CAN ID 0x%X", canId_);
            }

            return ret;
        }

        /**
         * @brief 使能电机
         * @return 错误码
         */
        appkit::ErrorCode Enable()
        {
            HwPackType pack{};
            pack.id = canId_ + static_cast<uint32_t>(controlMode_);
            pack.type = HwType::PackType::STANDARD;
            pack.data_len = 8;

            pack.data[0] = 0xFF;
            pack.data[1] = 0xFF;
            pack.data[2] = 0xFF;
            pack.data[3] = 0xFF;
            pack.data[4] = 0xFF;
            pack.data[5] = 0xFF;
            pack.data[6] = 0xFF;
            pack.data[7] = 0xFC;

            return hw_.AddMessage(pack);
        }

        /**
         * @brief 失能电机
         * @return 错误码
         */
        appkit::ErrorCode Disable()
        {
            HwPackType pack{};
            pack.id = canId_ + static_cast<uint32_t>(controlMode_);
            pack.type = HwType::PackType::STANDARD;
            pack.data_len = 8;

            pack.data[0] = 0xFF;
            pack.data[1] = 0xFF;
            pack.data[2] = 0xFF;
            pack.data[3] = 0xFF;
            pack.data[4] = 0xFF;
            pack.data[5] = 0xFF;
            pack.data[6] = 0xFF;
            pack.data[7] = 0xFD;

            return hw_.AddMessage(pack);
        }

        /**
         * @brief 保存当前位置为零点
         * @return 错误码
         */
        appkit::ErrorCode SavePosZero()
        {
            HwPackType pack{};
            pack.id = canId_ + static_cast<uint32_t>(controlMode_);
            pack.type = HwType::PackType::STANDARD;
            pack.data_len = 8;

            pack.data[0] = 0xFF;
            pack.data[1] = 0xFF;
            pack.data[2] = 0xFF;
            pack.data[3] = 0xFF;
            pack.data[4] = 0xFF;
            pack.data[5] = 0xFF;
            pack.data[6] = 0xFF;
            pack.data[7] = 0xFE;

            return hw_.AddMessage(pack);
        }

        /**
         * @brief 清除电机错误
         * @return 错误码
         */
        appkit::ErrorCode ClearError()
        {
            HwPackType pack{};
            pack.id = canId_ + static_cast<uint32_t>(controlMode_);
            pack.type = HwType::PackType::STANDARD;
            pack.data_len = 8;

            pack.data[0] = 0xFF;
            pack.data[1] = 0xFF;
            pack.data[2] = 0xFF;
            pack.data[3] = 0xFF;
            pack.data[4] = 0xFF;
            pack.data[5] = 0xFF;
            pack.data[6] = 0xFF;
            pack.data[7] = 0xFB;

            return hw_.AddMessage(pack);
        }

        /**
         * @brief MIT控制模式下的电机控制接口
         * @param pos 目标位置，单位：rad
         * @param vel 目标速度，单位：rad/s
         * @param kp 位置环比例系数
         * @param kd 位置环微分系数
         * @param tor 目标力矩，单位：Nm
         * @return 错误码
         */
        appkit::ErrorCode MitControl(float pos, float vel, float kp, float kd, float tor)
        {
            if (controlMode_ != ControlMode::MIT) [[unlikely]]
            {
                APPKIT_LOG_ERROR("Motor CAN ID 0x%X is not in MIT control mode", canId_);
                return appkit::ErrorCode::FAILED;
            }

            HwPackType pack{};
            pack.id = canId_ + static_cast<uint32_t>(ControlMode::MIT);
            pack.type = HwType::PackType::STANDARD;
            pack.data_len = 8;

            uint16_t pos_tmp = Float2Uint(pos, -pMax_, pMax_, 16);
            uint16_t vel_tmp = Float2Uint(vel, -vMax_, vMax_, 12);
            uint16_t tor_tmp = Float2Uint(tor, -tMax_, tMax_, 12);
            uint16_t kp_tmp = Float2Uint(kp, KP_MIN, KP_MAX, 12);
            uint16_t kd_tmp = Float2Uint(kd, KD_MIN, KD_MAX, 12);

            pack.data[0] = (pos_tmp >> 8);
            pack.data[1] = pos_tmp;
            pack.data[2] = (vel_tmp >> 4);
            pack.data[3] = ((vel_tmp & 0xF) << 4) | (kp_tmp >> 8);
            pack.data[4] = kp_tmp;
            pack.data[5] = (kd_tmp >> 4);
            pack.data[6] = ((kd_tmp & 0xF) << 4) | (tor_tmp >> 8);
            pack.data[7] = tor_tmp;

            return hw_.AddMessage(pack);
        }

        /**
         * @brief 位置速度控制模式下的电机控制接口
         * @param pos 目标位置，单位：rad
         * @param vel 目标速度，单位：rad/s
         * @return 错误码
         */
        appkit::ErrorCode PosVelControl(float pos, float vel)
        {
            if (controlMode_ != ControlMode::POS_VEL) [[unlikely]]
            {
                APPKIT_LOG_ERROR("Motor CAN ID 0x%X is not in POS_VEL control mode", canId_);
                return appkit::ErrorCode::FAILED;
            }

            HwPackType pack{};
            pack.id = canId_ + static_cast<uint32_t>(ControlMode::POS_VEL);
            pack.type = HwType::PackType::STANDARD;
            pack.data_len = 8;

            appkit::Memory::Copy(pack.data, &pos, sizeof(float));
            appkit::Memory::Copy(pack.data + 4, &vel, sizeof(float));

            return hw_.AddMessage(pack);
        }

        /**
         * @brief 速度控制模式下的电机控制接口
         * @param vel 目标速度，单位：rad/s
         * @return 错误码
         */
        appkit::ErrorCode VelControl(float vel)
        {
            if (controlMode_ != ControlMode::VEL) [[unlikely]]
            {
                APPKIT_LOG_ERROR("Motor CAN ID 0x%X is not in VEL control mode", canId_);
                return appkit::ErrorCode::FAILED;
            }

            HwPackType pack{};
            pack.id = canId_ + static_cast<uint32_t>(ControlMode::VEL);
            pack.type = HwType::PackType::STANDARD;
            pack.data_len = 4;

            appkit::Memory::Copy(pack.data, &vel, sizeof(float));

            return hw_.AddMessage(pack);
        }

        /**
         * @brief 位置力矩控制模式下的电机控制接口
         * @param pos 目标位置，单位：rad
         * @param vel 目标速度，单位：rad/s,范围[0,100]
         * @param cur 目标电流，单位：%FS,  范围[0,1.0]
         * @return 错误码
         */
        appkit::ErrorCode PosForceControl(float pos, float vel, float cur)
        {
            if (controlMode_ != ControlMode::POS_FORCE) [[unlikely]]
            {
                APPKIT_LOG_ERROR("Motor CAN ID 0x%X is not in POS_FORCE control mode", canId_);
                return appkit::ErrorCode::FAILED;
            }

            HwPackType pack{};
            pack.id = canId_ + static_cast<uint32_t>(ControlMode::POS_FORCE);
            pack.type = HwType::PackType::STANDARD;
            pack.data_len = 8;

            uint16_t u16_vel = static_cast<uint16_t>(std::clamp(vel, 0.0f, 100.0f) * 100);
            uint16_t u16_cur = static_cast<uint16_t>(std::clamp(cur, 0.0f, 1.0f) * 10000);

            appkit::Memory::Copy(pack.data, &pos, sizeof(float));
            appkit::Memory::Copy(pack.data + 4, &u16_vel, sizeof(uint16_t));
            appkit::Memory::Copy(pack.data + 6, &u16_cur, sizeof(uint16_t));

            return hw_.AddMessage(pack);
        }

        /**
         * @brief 力矩控制模式下的电机控制接口（重载）
         * @param tor 目标力矩，单位：Nm
         * @return 错误码
         */
        appkit::ErrorCode TorqueControl(float tor) override
        {
            return MitControl(0.0f, 0.0f, 0.0f, 0.0f, tor);
        }

        /**
         * @brief 获取电机测量数据接口
         * @return 电机测量数据引用
         */
        const Measure &GetMeasure() const override
        {
            return measure_;
        }

    private:
        uint32_t canId_ = 0;      ///< CAN ID
        uint32_t feedbackId_ = 0; ///< 反馈ID
        Type motorType_;          ///< 电机类型
        ControlMode controlMode_; ///< 控制模式

        float pMax_ = 0; ///< 位置映射范围
        float vMax_ = 0; ///< 速度映射范围
        float tMax_ = 0; ///< 力矩映射范围

        HwType hw_{}; ///< 硬件接口对象

        Measure measure_{};        ///< 电机测量数据
        FeedbackFrame feedback_{}; ///< 电机反馈数据帧
        uint32_t cycle_count_ = 0; ///< 电机位置周期计数

        /**
         * @brief 解码接收到的CAN数据帧
         * @param in_isr 是否在中断服务程序中
         * @param motor 电机对象指针
         * @param frame 接收到的CAN数据帧引用
         */
        static void DecodeData(bool in_isr, DMMotor *motor, const HwPackType &frame)
        {
            UNUSED(in_isr);

            if (frame.id == motor->feedbackId_)
            {
                appkit::Memory::Copy(&motor->feedback_, frame.data, sizeof(FeedbackFrame));

                // 计算绝对位置和单圈位置
                float last_pos = motor->measure_.abs_pos;
                float current_pos = Uint2Float(motor->feedback_.position, -motor->pMax_, motor->pMax_, 16);
                if (current_pos - last_pos > motor->pMax_ / 2)
                {
                    motor->cycle_count_--;
                }
                else if (current_pos - last_pos < -motor->pMax_ / 2)
                {
                    motor->cycle_count_++;
                }
                motor->measure_.abs_pos = current_pos + motor->cycle_count_ * (2.0f * motor->pMax_);
                motor->measure_.single_pos = current_pos;

                // 解析速度、力矩和温度数据
                motor->measure_.speed = Uint2Float(motor->feedback_.speed, -motor->vMax_, motor->vMax_, 12);
                motor->measure_.torque = Uint2Float(motor->feedback_.torque, -motor->tMax_, motor->tMax_, 12);
                motor->measure_.temperature = static_cast<float>(motor->feedback_.rotor_temp);
            }
            else if (frame.id == BOARDCAST_ID)
            {
                // 寄存器反馈帧 || 其他电机寄存器读写
                const RegFrameNoVal &reg_frame_header = *appkit::ConstRawData(frame.data).GetData<RegFrameNoVal>();

                // 判断是否是针对本电机的寄存器操作
                if (reg_frame_header.can_id == motor.canId_)
                {
                    switch (static_cast<Register>(reg_frame_header.reg_addr))
                    {
                    case Register::PMAX:
                    {
                        const RegFrame<float> &reg_value = *appkit::ConstRawData(frame.data).GetData<RegFrame<float>>();

                        // 检查是否在合理范围内
                        if (reg_value.value > 0.0f && reg_value.value <= std::numeric_limits<float>::max())
                        {
                            motor->pMax_ = reg_value.value;
                        }
                        break;
                    }
                    case Register::VMAX:
                    {
                        const RegFrame<float> &reg_value = *appkit::ConstRawData(frame.data).GetData<RegFrame<float>>();

                        // 检查是否在合理范围内
                        if (reg_value.value > 0.0f && reg_value.value <= std::numeric_limits<float>::max())
                        {
                            motor->vMax_ = reg_value.value;
                        }
                        break;
                    }
                    case Register::TMAX:
                    {
                        const RegFrame<float> &reg_value = *appkit::ConstRawData(frame.data).GetData<RegFrame<float>>();

                        // 检查是否在合理范围内
                        if (reg_value.value > 0.0f && reg_value.value <= std::numeric_limits<float>::max())
                        {
                            motor->tMax_ = reg_value.value;
                        }
                        break;
                    }
                    default:
                        // 其他寄存器读写处理
                        break;
                    }
                }
            }
        }
    };
} // namespace control::motor
