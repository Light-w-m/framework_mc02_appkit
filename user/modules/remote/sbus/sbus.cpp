#ifdef ENABLED_MODULES_SBUS

#include <sbus.hpp>

#include <logger.hpp>

/**
 * @brief SBUS帧间隔时间（单位：毫秒）
 */
#ifndef SBUS_FRAME_PERIOD_MS
#define SBUS_FRAME_PERIOD_MS 14
#pragma message("SBUS_FRAME_PERIOD_MS not defined, default to 14ms")
#else
static_assert(SBUS_FRAME_PERIOD_MS >= 4 && SBUS_FRAME_PERIOD_MS <= 14, "SBUS_FRAME_PERIOD_MS out of valid range (4-14ms)");
#endif

// 检查是否定义了摇杆通道映射宏
#if defined(SBUS_CHANNEL_ROCKER_L1) || defined(SBUS_CHANNEL_ROCKER_L_) || defined(SBUS_CHANNEL_ROCKER_R1) || defined(SBUS_CHANNEL_ROCKER_R_)
#define SBUS_CHANNEL_ROCKER_DEFINED
#endif

// 检查是否定义了二挡开关通道映射宏
#if defined(SBUS_CHANNEL_SWITCH_D1) || defined(SBUS_CHANNEL_SWITCH_D2) || defined(SBUS_CHANNEL_SWITCH_D3) || defined(SBUS_CHANNEL_SWITCH_D4)
#define SBUS_CHANNEL_SWITCH_D_DEFINED
#endif

// 检查是否定义了三挡开关通道映射宏
#if defined(SBUS_CHANNEL_SWITCH_T1) || defined(SBUS_CHANNEL_SWITCH_T2) || defined(SBUS_CHANNEL_SWITCH_T3) || defined(SBUS_CHANNEL_SWITCH_T4)
#define SBUS_CHANNEL_SWITCH_T_DEFINED
#endif

// 检查是否定义了附加摇杆通道映射宏
#if defined(SBUS_CHANNEL_ROCKER_E1) || defined(SBUS_CHANNEL_ROCKER_E2)
#define SBUS_CHANNEL_ROCKER_E_DEFINED
#endif

// 检查是否至少定义了一个通道映射宏
#if !defined(SBUS_CHANNEL_ROCKER_DEFINED) && !defined(SBUS_CHANNEL_SWITCH_D_DEFINED) && !defined(SBUS_CHANNEL_SWITCH_T_DEFINED) && !defined(SBUS_CHANNEL_ROCKER_E_DEFINED)
#warning "No Sbus channel mapping macros defined, Sbus module will not function properly"
#endif

constexpr appkit::Duration Sbus::FRAME_PERIOD = appkit::Duration::From<appkit::millisecond>(SBUS_FRAME_PERIOD_MS);

int8_t Sbus::GetSwitchDValue(int16_t channel_value) noexcept
{
    return (channel_value == CHANNEL_MAX) ? 1 : -1;
}

int8_t Sbus::GetSwitchTValue(int16_t channel_value) noexcept
{
    return (channel_value == CHANNEL_MIDPOINT) ? 0 : ((channel_value == CHANNEL_MAX) ? 1 : -1);
}

bool Sbus::CheckChannelValid() const noexcept
{
    // 检查摇杆通道值是否在合理范围内
#if defined(SBUS_CHANNEL_ROCKER_DEFINED) || defined(SBUS_CHANNEL_ROCKER_E_DEFINED)
    static constexpr uint16_t min_rocker_value = CHANNEL_MIN;
    static constexpr uint16_t max_rocker_value = CHANNEL_MAX;

    if (
#ifdef SBUS_CHANNEL_ROCKER_L1
        (packet_.channel_data[SBUS_CHANNEL_ROCKER_L1] < min_rocker_value) || (packet_.channel_data[SBUS_CHANNEL_ROCKER_L1] > max_rocker_value) ||
#endif
#ifdef SBUS_CHANNEL_ROCKER_L_
        (packet_.channel_data[SBUS_CHANNEL_ROCKER_L_] < min_rocker_value) || (packet_.channel_data[SBUS_CHANNEL_ROCKER_L_] > max_rocker_value) ||
#endif
#ifdef SBUS_CHANNEL_ROCKER_R1
        (packet_.channel_data[SBUS_CHANNEL_ROCKER_R1] < min_rocker_value) || (packet_.channel_data[SBUS_CHANNEL_ROCKER_R1] > max_rocker_value) ||
#endif
#ifdef SBUS_CHANNEL_ROCKER_R_
        (packet_.channel_data[SBUS_CHANNEL_ROCKER_R_] < min_rocker_value) || (packet_.channel_data[SBUS_CHANNEL_ROCKER_R_] > max_rocker_value) ||
#endif
#ifdef SBUS_CHANNEL_ROCKER_E1
        (packet_.channel_data[SBUS_CHANNEL_ROCKER_E1] < min_rocker_value) || (packet_.channel_data[SBUS_CHANNEL_ROCKER_E1] > max_rocker_value) ||
#endif
#ifdef SBUS_CHANNEL_ROCKER_E2
        (packet_.channel_data[SBUS_CHANNEL_ROCKER_E2] < min_rocker_value) || (packet_.channel_data[SBUS_CHANNEL_ROCKER_E2] > max_rocker_value) ||
#endif
        false)
    {
        return false;
    }
#endif

    // 检查二挡开关值是否在合理范围内
#ifdef SBUS_CHANNEL_SWITCH_D_DEFINED
    static constexpr int16_t min_switch_d_value = CHANNEL_MIN;
    static constexpr int16_t max_switch_d_value = CHANNEL_MAX;

    if (
#ifdef SBUS_CHANNEL_SWITCH_D1
        ((packet_.channel_data[SBUS_CHANNEL_SWITCH_D1] != min_switch_d_value) && (packet_.channel_data[SBUS_CHANNEL_SWITCH_D1] != max_switch_d_value)) ||
#endif
#ifdef SBUS_CHANNEL_SWITCH_D2
        ((packet_.channel_data[SBUS_CHANNEL_SWITCH_D2] != min_switch_d_value) && (packet_.channel_data[SBUS_CHANNEL_SWITCH_D2] != max_switch_d_value)) ||
#endif
#ifdef SBUS_CHANNEL_SWITCH_D3
        ((packet_.channel_data[SBUS_CHANNEL_SWITCH_D3] != min_switch_d_value) && (packet_.channel_data[SBUS_CHANNEL_SWITCH_D3] != max_switch_d_value)) ||
#endif
#ifdef SBUS_CHANNEL_SWITCH_D4
        ((packet_.channel_data[SBUS_CHANNEL_SWITCH_D4] != min_switch_d_value) && (packet_.channel_data[SBUS_CHANNEL_SWITCH_D4] != max_switch_d_value)) ||
#endif
        false)
    {
        return false;
    }
#endif

    // 检查三挡开关值是否在合理范围内
#ifdef SBUS_CHANNEL_SWITCH_T_DEFINED
    static constexpr int16_t min_switch_t_value = CHANNEL_MIN;
    static constexpr int16_t mid_switch_t_value = CHANNEL_MIDPOINT;
    static constexpr int16_t max_switch_t_value = CHANNEL_MAX;

    if (
#ifdef SBUS_CHANNEL_SWITCH_T1
        ((packet_.channel_data[SBUS_CHANNEL_SWITCH_T1] != min_switch_t_value) && (packet_.channel_data[SBUS_CHANNEL_SWITCH_T1] != mid_switch_t_value) &&
         (packet_.channel_data[SBUS_CHANNEL_SWITCH_T1] != max_switch_t_value)) ||
#endif
#ifdef SBUS_CHANNEL_SWITCH_T2
        ((packet_.channel_data[SBUS_CHANNEL_SWITCH_T2] != min_switch_t_value) && (packet_.channel_data[SBUS_CHANNEL_SWITCH_T2] != mid_switch_t_value) &&
         (packet_.channel_data[SBUS_CHANNEL_SWITCH_T2] != max_switch_t_value)) ||
#endif
#ifdef SBUS_CHANNEL_SWITCH_T3
        ((packet_.channel_data[SBUS_CHANNEL_SWITCH_T3] != min_switch_t_value) && (packet_.channel_data[SBUS_CHANNEL_SWITCH_T3] != mid_switch_t_value) &&
         (packet_.channel_data[SBUS_CHANNEL_SWITCH_T3] != max_switch_t_value)) ||
#endif
#ifdef SBUS_CHANNEL_SWITCH_T4
        ((packet_.channel_data[SBUS_CHANNEL_SWITCH_T4] != min_switch_t_value) && (packet_.channel_data[SBUS_CHANNEL_SWITCH_T4] != mid_switch_t_value) &&
         (packet_.channel_data[SBUS_CHANNEL_SWITCH_T4] != max_switch_t_value)) ||
#endif
        false)
    {
        return false;
    }
#endif

    // 检查帧尾是否正确
    if ((packet_.end_byte != Sbus::SBUS_END_BYTE_1) && (packet_.end_byte != Sbus::SBUS_END_BYTE_2))
    {
        return false;
    }

    return true;
}

void Sbus::ParseData() noexcept
{
#if defined(SBUS_CHANNEL_ROCKER_DEFINED) || defined(SBUS_CHANNEL_ROCKER_E_DEFINED)
    static constexpr float CHANNEL_SCALE = 2.0f / (CHANNEL_MAX - CHANNEL_MIN); ///< 通道缩放因子 [CHANNEL_MIN, CHANNEL_MAX] -> [-1, +1]
#endif

    // 解析遥控器数据
    [[maybe_unused]] auto &data = remoteData_.data;

    // 摇杆通道
#ifdef SBUS_CHANNEL_ROCKER_DEFINED
#ifdef SBUS_CHANNEL_ROCKER_L1
    data.rocker_l1 = (static_cast<float>(packet_.channel_data[SBUS_CHANNEL_ROCKER_L1] - CHANNEL_MIDPOINT)) * CHANNEL_SCALE;
#endif
#ifdef SBUS_CHANNEL_ROCKER_L_
    data.rocker_l_ = (static_cast<float>(packet_.channel_data[SBUS_CHANNEL_ROCKER_L_] - CHANNEL_MIDPOINT)) * CHANNEL_SCALE;
#endif
#ifdef SBUS_CHANNEL_ROCKER_R1
    data.rocker_r1 = (static_cast<float>(packet_.channel_data[SBUS_CHANNEL_ROCKER_R1] - CHANNEL_MIDPOINT)) * CHANNEL_SCALE;
#endif
#ifdef SBUS_CHANNEL_ROCKER_R_
    data.rocker_r_ = (static_cast<float>(packet_.channel_data[SBUS_CHANNEL_ROCKER_R_] - CHANNEL_MIDPOINT)) * CHANNEL_SCALE;
#endif
#endif

    // 二挡开关通道
#ifdef SBUS_CHANNEL_SWITCH_D_DEFINED
#ifdef SBUS_CHANNEL_SWITCH_D1
    data.switch_d1 = GetSwitchDValue(packet_.channel_data[SBUS_CHANNEL_SWITCH_D1]);
#endif
#ifdef SBUS_CHANNEL_SWITCH_D2
    data.switch_d2 = GetSwitchDValue(packet_.channel_data[SBUS_CHANNEL_SWITCH_D2]);
#endif
#ifdef SBUS_CHANNEL_SWITCH_D3
    data.switch_d3 = GetSwitchDValue(packet_.channel_data[SBUS_CHANNEL_SWITCH_D3]);
#endif
#ifdef SBUS_CHANNEL_SWITCH_D4
    data.switch_d4 = GetSwitchDValue(packet_.channel_data[SBUS_CHANNEL_SWITCH_D4]);
#endif
#endif

// 三挡开关通道
#ifdef SBUS_CHANNEL_SWITCH_T_DEFINED
#ifdef SBUS_CHANNEL_SWITCH_T1
    data.switch_t1 = GetSwitchTValue(packet_.channel_data[SBUS_CHANNEL_SWITCH_T1]);
#endif
#ifdef SBUS_CHANNEL_SWITCH_T2
    data.switch_t2 = GetSwitchTValue(packet_.channel_data[SBUS_CHANNEL_SWITCH_T2]);
#endif
#ifdef SBUS_CHANNEL_SWITCH_T3
    data.switch_t3 = GetSwitchTValue(packet_.channel_data[SBUS_CHANNEL_SWITCH_T3]);
#endif
#ifdef SBUS_CHANNEL_SWITCH_T4
    data.switch_t4 = GetSwitchTValue(packet_.channel_data[SBUS_CHANNEL_SWITCH_T4]);
#endif
#endif

    // 附加摇杆通道
#ifdef SBUS_CHANNEL_ROCKER_E_DEFINED
#ifdef SBUS_CHANNEL_ROCKER_E1
    data.rocker_e1 = (static_cast<float>(packet_.channel_data[SBUS_CHANNEL_ROCKER_E1] - CHANNEL_MIDPOINT)) * CHANNEL_SCALE;
#endif
#ifdef SBUS_CHANNEL_ROCKER_E2
    data.rocker_e2 = (static_cast<float>(packet_.channel_data[SBUS_CHANNEL_ROCKER_E2] - CHANNEL_MIDPOINT)) * CHANNEL_SCALE;
#endif
#endif
}

void Sbus::ReadThreadFunc(Sbus *self)
{
    auto &read_port = *self->uart_.GetReadPort();
    uint8_t buffer[24] = {0}; ///< 数据缓冲区

    for (;;)
    {
        // 默认事件标志为离线
        uint32_t event_flags = static_cast<uint32_t>(EventFlag::OFFLINE);

        // 获取已接收到的数据长度
        UNUSED(read_port({nullptr, 0}, self->op_));
        const auto available_size = read_port.buffer_->Available();

        // 检查是否有足够的数据包长度
        if (available_size >= Sbus::SBUS_RAW_FRAME_SIZE)
        {
            // 读取帧头
            uint8_t &start_byte = self->packet_.start_byte;
            UNUSED(read_port(appkit::RawData{start_byte}, self->op_));

            // 检查帧头是否正确
            if (start_byte == Sbus::SBUS_START_BYTE)
            {
                // 读取剩余数据包
                UNUSED(read_port(appkit::RawData{buffer}, self->op_));

                // 复制数据到数据包结构体
                auto &packet = self->packet_;

                packet.channel_data[0] = static_cast<int16_t>(buffer[0]) | ((buffer[1] << 8) & 0x07FF);
                packet.channel_data[1] = static_cast<int16_t>(buffer[1] >> 3) | ((buffer[2] << 5) & 0x07FF);
                packet.channel_data[2] = static_cast<int16_t>(buffer[2] >> 6) | (buffer[3] << 2) | ((buffer[4] << 10) & 0x07FF);
                packet.channel_data[3] = static_cast<int16_t>(buffer[4] >> 1) | ((buffer[5] << 7) & 0x07FF);
                packet.channel_data[4] = static_cast<int16_t>(buffer[5] >> 4) | ((buffer[6] << 4) & 0x07FF);
                packet.channel_data[5] = static_cast<int16_t>(buffer[6] >> 7) | (buffer[7] << 1) | ((buffer[8] << 9) & 0x07FF);
                packet.channel_data[6] = static_cast<int16_t>(buffer[8] >> 2) | ((buffer[9] << 6) & 0x07FF);
                packet.channel_data[7] = static_cast<int16_t>(buffer[9] >> 5) | ((buffer[10] << 3) & 0x07FF);
                packet.channel_data[8] = static_cast<int16_t>(buffer[11]) | ((buffer[12] << 8) & 0x07FF);
                packet.channel_data[9] = static_cast<int16_t>(buffer[12] >> 3) | ((buffer[13] << 5) & 0x07FF);
                packet.channel_data[10] = static_cast<int16_t>(buffer[13] >> 6) | ((buffer[14] << 2) & 0x07FF) | ((buffer[15] << 10) & 0x07FF);
                packet.channel_data[11] = static_cast<int16_t>(buffer[15] >> 1) | ((buffer[16] << 7) & 0x07FF);
                packet.channel_data[12] = static_cast<int16_t>(buffer[16] >> 4) | ((buffer[17] << 4) & 0x07FF);
                packet.channel_data[13] = static_cast<int16_t>(buffer[17] >> 7) | ((buffer[18] << 1) & 0x07FF) | ((buffer[19] << 9) & 0x07FF);
                packet.channel_data[14] = static_cast<int16_t>(buffer[19] >> 2) | ((buffer[20] << 6) & 0x07FF);
                packet.channel_data[15] = static_cast<int16_t>(buffer[20] >> 5) | ((buffer[21] << 3) & 0x07FF);

                packet.flags.ch17 = buffer[22] & 0x01;
                packet.flags.ch16 = (buffer[22] >> 1) & 0x01;
                packet.flags.frame_lost = (buffer[22] >> 2) & 0x01;
                packet.flags.failsafe = (buffer[22] >> 3) & 0x01;

                packet.end_byte = buffer[23];

                // 检查数据有效性
                if (self->CheckChannelValid())
                {
                    // 检查协议标志位
                    if (packet.flags.frame_lost)
                    {
                        // 设置帧丢失事件标志
                        event_flags |= static_cast<uint32_t>(EventFlag::FRAME_LOST);
                        APPKIT_LOG_WARNING("SBUS frame lost detected");
                    }

                    if (packet.flags.failsafe)
                    {
                        // 设置失控保护事件标志
                        event_flags |= static_cast<uint32_t>(EventFlag::FAILSAFE);
                        APPKIT_LOG_WARNING("SBUS failsafe activated");
                    }

                    // 解析遥控器数据
                    self->ParseData();

                    // 发布数据
                    event_flags |= static_cast<uint32_t>(EventFlag::ONLINE);
                    self->remoteTopic_.Publish(self->remoteData_);
                    self->remoteEvent_.Activate(event_flags);

                    // 进行下一轮循环
                    self->lostCount_ = 0;
                    goto continue_wrap;
                }
            }
        }

        // 数据丢失处理 5帧未收到有效数据则认为离线
        if (++self->lostCount_ > 5)
        {
            self->remoteEvent_.Activate(static_cast<uint32_t>(EventFlag::OFFLINE));
            APPKIT_LOG_WARNING("SBUS remote offline due to data loss");
            self->lostCount_ = 0;
        }

        read_port.buffer_->Reset();

    continue_wrap:
        // 等待下一次读取
        appkit::osal::this_thread::SleepFor(FRAME_PERIOD / 4); // 等待1/4帧数据时间
    }
}

Sbus::Sbus(const char *uart_name, uint32_t read_thread_stack_size)
    : op_{dataSem_, FRAME_PERIOD}
{
    // 初始化UART
    APPKIT_RAISE_IF_NOT(appkit::Check(uart_.Open(uart_name)),
                        "Failed to open UART device");
    APPKIT_RAISE_IF_NOT(uart_.GetReadPort()->Readable(),
                        "UART read port is not readable");
    APPKIT_RAISE_IF_NOT(uart_.GetReadPort()->buffer_->Capacity() >= SBUS_RAW_FRAME_SIZE,
                        "UART read buffer size is insufficient");

    // 创建话题
    appkit::Topic::Domain domain("control");
    remoteTopic_ = appkit::Topic::CreateTopic<control::RemoteCtrlData>("sbus_remote", &domain);

    // 创建事件
    APPKIT_RAISE_IF_NOT(appkit::Check(remoteEvent_.Open("sbus")),
                        "Failed to create remote event");

    // 创建读取线程
    readThread_.Create(
        ReadThreadFunc, this,
        "sbus_read",
        read_thread_stack_size, appkit::osal::Thread::Priority::HIGH);
}

/**
 * @brief 用于检查摇杆通道映射宏索引范围的辅助类
 */
struct SbusIndexChecker final
{
// 检查摇杆通道映射宏是否在有效范围内
#ifdef SBUS_CHANNEL_ROCKER_L1
    static_assert(SBUS_CHANNEL_ROCKER_L1 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_ROCKER_L1 index out of range");
#endif
#ifdef SBUS_CHANNEL_ROCKER_L_
    static_assert(SBUS_CHANNEL_ROCKER_L_ < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_ROCKER_L_ index out of range");
#endif
#ifdef SBUS_CHANNEL_ROCKER_R1
    static_assert(SBUS_CHANNEL_ROCKER_R1 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_ROCKER_R1 index out of range");
#endif
#ifdef SBUS_CHANNEL_ROCKER_R_
    static_assert(SBUS_CHANNEL_ROCKER_R_ < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_ROCKER_R_ index out of range");
#endif

// 检查二挡开关通道映射宏是否在有效范围内
#ifdef SBUS_CHANNEL_SWITCH_D1
    static_assert(SBUS_CHANNEL_SWITCH_D1 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_SWITCH_D1 index out of range");
#endif
#ifdef SBUS_CHANNEL_SWITCH_D2
    static_assert(SBUS_CHANNEL_SWITCH_D2 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_SWITCH_D2 index out of range");
#endif
#ifdef SBUS_CHANNEL_SWITCH_D3
    static_assert(SBUS_CHANNEL_SWITCH_D3 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_SWITCH_D3 index out of range");
#endif
#ifdef SBUS_CHANNEL_SWITCH_D4
    static_assert(SBUS_CHANNEL_SWITCH_D4 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_SWITCH_D4 index out of range");
#endif

// 检查三挡开关通道映射宏是否在有效范围内
#ifdef SBUS_CHANNEL_SWITCH_T1
    static_assert(SBUS_CHANNEL_SWITCH_T1 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_SWITCH_T1 index out of range");
#endif
#ifdef SBUS_CHANNEL_SWITCH_T2
    static_assert(SBUS_CHANNEL_SWITCH_T2 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_SWITCH_T2 index out of range");
#endif
#ifdef SBUS_CHANNEL_SWITCH_T3
    static_assert(SBUS_CHANNEL_SWITCH_T3 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_SWITCH_T3 index out of range");
#endif
#ifdef SBUS_CHANNEL_SWITCH_T4
    static_assert(SBUS_CHANNEL_SWITCH_T4 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_SWITCH_T4 index out of range");
#endif

// 检查附加摇杆通道映射宏是否在有效范围内
#ifdef SBUS_CHANNEL_ROCKER_E1
    static_assert(SBUS_CHANNEL_ROCKER_E1 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_ROCKER_E1 index out of range");
#endif
#ifdef SBUS_CHANNEL_ROCKER_E2
    static_assert(SBUS_CHANNEL_ROCKER_E2 < Sbus::SBUS_CHANNEL_COUNT, "SBUS_CHANNEL_ROCKER_E2 index out of range");
#endif
};

#endif // ENABLED_MODULES_SBUS