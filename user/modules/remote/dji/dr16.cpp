#ifdef ENABLED_MODULES_DR16

#include <dr16.hpp>

#include <logger.hpp>

using namespace appkit::time_literals;

DR16::DR16(const char *uart_name, uint32_t read_thread_stack_size)
{
    // 初始化UART
    APPKIT_RAISE_IF_NOT(appkit::Check(uart_.Open(uart_name)),
                        "Failed to open UART device");
    APPKIT_RAISE_IF_NOT(uart_.GetReadPort()->Readable(),
                        "UART read port is not readable");
    APPKIT_RAISE_IF_NOT(uart_.GetReadPort()->buffer_->Capacity() >= sizeof(DR16Packet),
                        "UART read buffer size is insufficient");

    // 创建话题
    appkit::Topic::Domain domain("control");
    remoteTopic_ = appkit::Topic::CreateTopic<control::RemoteCtrlData>("dr16_remote", &domain);
    keymouseTopic_ = appkit::Topic::CreateTopic<control::KeymouseCtrlData>("dr16_keymouse", &domain);

    // 创建事件
    APPKIT_RAISE_IF_NOT(appkit::Check(remoteEvent_.Open("dr16")),
                        "Failed to create remote event");

    // 创建读取线程
    readThread_.Create(
        ReadThreadFunc, this, 
        "dr16_read",
        read_thread_stack_size, appkit::osal::Thread::Priority::HIGH);
}

bool DR16::CheckChannelValid() const noexcept
{
    // 检查摇杆通道值是否在合理范围内
    static constexpr uint16_t min_rocker_value = CHANNEL_MIN;
    static constexpr uint16_t max_rocker_value = CHANNEL_MAX;

    if ((rawData_.channel_0 < min_rocker_value) || (rawData_.channel_0 > max_rocker_value) ||
        (rawData_.channel_1 < min_rocker_value) || (rawData_.channel_1 > max_rocker_value) ||
        (rawData_.channel_2 < min_rocker_value) || (rawData_.channel_2 > max_rocker_value) ||
        (rawData_.channel_3 < min_rocker_value) || (rawData_.channel_3 > max_rocker_value))
    {
        return false;
    }

    // 检查开关值是否在合理范围内
    static constexpr uint8_t min_switch_value = 1;
    static constexpr uint8_t max_switch_value = 3;

    if ((rawData_.s1 < min_switch_value) || (rawData_.s1 > max_switch_value) ||
        (rawData_.s2 < min_switch_value) || (rawData_.s2 > max_switch_value))
    {
        return false;
    }

    // 检查鼠标按键值是否在合理范围内
    static constexpr uint8_t min_mouse_button_value = 0;
    static constexpr uint8_t max_mouse_button_value = 1;

    if ((rawData_.press_l < min_mouse_button_value) || (rawData_.press_l > max_mouse_button_value) ||
        (rawData_.press_r < min_mouse_button_value) || (rawData_.press_r > max_mouse_button_value))
    {
        return false;
    }

    // 检查拨轮值是否在合理范围内
    static constexpr uint16_t min_dial_value = CHANNEL_MIN;
    static constexpr uint16_t max_dial_value = CHANNEL_MAX;

    if ((rawData_.dial < min_dial_value) || (rawData_.dial > max_dial_value))
    {
        return false;
    }

    return true;
}

void DR16::ParseData() noexcept
{
    static constexpr float CHANNEL_SCALE = 2.0f / (CHANNEL_MAX - CHANNEL_MIN); ///< 通道缩放因子 [CHANNEL_MIN, CHANNEL_MAX] -> [-1, +1]
    static constexpr int8_t SWITCH_OFFSET = 2;                                 ///< 开关偏移量 {1, 2, 3} -> {-1, 0, 1}
    static constexpr float MOUSE_SPEED_SCALE = 1.0f / INT16_MAX;               ///< 鼠标速度缩放因子 [INT16_MIN, INT16_MAX] -> [-1, +1]

    // 解析遥控器摇杆通道数据
    remoteData_.data.rocker_l_ = (static_cast<int16_t>(rawData_.channel_0) - CHANNEL_MIDPOINT) * CHANNEL_SCALE;
    remoteData_.data.rocker_l1 = (static_cast<int16_t>(rawData_.channel_1) - CHANNEL_MIDPOINT) * CHANNEL_SCALE;
    remoteData_.data.rocker_r_ = (static_cast<int16_t>(rawData_.channel_2) - CHANNEL_MIDPOINT) * CHANNEL_SCALE;
    remoteData_.data.rocker_r1 = (static_cast<int16_t>(rawData_.channel_3) - CHANNEL_MIDPOINT) * CHANNEL_SCALE;
    remoteData_.data.rocker_e1 = (static_cast<int16_t>(rawData_.dial) - CHANNEL_MIDPOINT) * CHANNEL_SCALE;

    // 解析遥控器开关数据
    remoteData_.data.switch_t1 = static_cast<int8_t>(rawData_.s1) - SWITCH_OFFSET;
    remoteData_.data.switch_t2 = static_cast<int8_t>(rawData_.s2) - SWITCH_OFFSET;

    // 解析鼠标数据
    keymouseData_.data.mouse_speed_x = rawData_.mouse_x * MOUSE_SPEED_SCALE;
    keymouseData_.data.mouse_speed_y = rawData_.mouse_y * MOUSE_SPEED_SCALE;
    keymouseData_.data.mouse_speed_z = rawData_.mouse_z * MOUSE_SPEED_SCALE;
    keymouseData_.data.press_l = static_cast<bool>(rawData_.press_l);
    keymouseData_.data.press_r = static_cast<bool>(rawData_.press_r);

    // 解析键盘数据
    keymouseData_.data.w = rawData_.key.w;
    keymouseData_.data.s = rawData_.key.s;
    keymouseData_.data.d = rawData_.key.d;
    keymouseData_.data.a = rawData_.key.a;
    keymouseData_.data.shift = rawData_.key.shift;
    keymouseData_.data.ctrl = rawData_.key.ctrl;
    keymouseData_.data.q = rawData_.key.q;
    keymouseData_.data.e = rawData_.key.e;
    keymouseData_.data.r = rawData_.key.r;
    keymouseData_.data.f = rawData_.key.f;
    keymouseData_.data.g = rawData_.key.g;
    keymouseData_.data.z = rawData_.key.z;
    keymouseData_.data.x = rawData_.key.x;
    keymouseData_.data.c = rawData_.key.c;
    keymouseData_.data.v = rawData_.key.v;
    keymouseData_.data.b = rawData_.key.b;
}

void DR16::ReadThreadFunc(DR16 *self)
{
    static constexpr size_t packet_size = sizeof(DR16Packet);
    static uint8_t packet_buffer[6];

    auto &read_port = *self->uart_.GetReadPort();

    for (;;)
    {
        // 获取已接收到的数据长度
        UNUSED(read_port({nullptr, 0}, self->op_));
        const auto available_size = read_port.buffer_->Available();

        // 检查是否有足够的数据包长度
        if (available_size >= packet_size)
        {
            // 读取数据包
            // 读取通道和开关数据 6字节
            UNUSED(read_port({packet_buffer, 6}, self->op_));
            self->rawData_.channel_0 = (static_cast<uint16_t>(packet_buffer[0]) | (static_cast<uint16_t>(packet_buffer[1] & 0x07) << 8));
            self->rawData_.channel_1 = ((static_cast<uint16_t>(packet_buffer[1] >> 3) | (static_cast<uint16_t>(packet_buffer[2] & 0x3F) << 5)));
            self->rawData_.channel_2 = ((static_cast<uint16_t>(packet_buffer[2] >> 6) | (static_cast<uint16_t>(packet_buffer[3] & 0xFF) << 2) |
                                         (static_cast<uint16_t>(packet_buffer[4] & 0x01) << 10)));
            self->rawData_.channel_3 = ((static_cast<uint16_t>(packet_buffer[4] >> 1) | (static_cast<uint16_t>(packet_buffer[5] & 0x0F) << 7)));
            self->rawData_.s1 = ((packet_buffer[5] >> 4) & 0x03);
            self->rawData_.s2 = ((packet_buffer[5] >> 6) & 0x03);

            // 读取鼠标、按键数据 10字节
            UNUSED(read_port({&self->rawData_.mouse_x, 10}, self->op_));

            // 读取拨轮数据 2字节
            UNUSED(read_port({packet_buffer, 2}, self->op_));
            self->rawData_.dial = static_cast<uint16_t>(packet_buffer[0] | (packet_buffer[1] << 8));

            // 检查数据有效性
            if (self->CheckChannelValid())
            {
                // 解析遥控器数据
                self->ParseData();

                // 发布数据
                self->remoteTopic_.Publish(self->remoteData_);
                self->keymouseTopic_.Publish(self->keymouseData_);
                self->remoteEvent_.Activate(static_cast<uint32_t>(EventFlag::ONLINE));

                // 进行下一轮循环
                self->lostCount_ = 0;
                goto continue_wrap;
            }
        }

        // 数据丢失处理 5帧未收到有效数据则认为离线
        if (++self->lostCount_ > 20)
        {
            self->remoteEvent_.Activate(static_cast<uint32_t>(EventFlag::OFFLINE));
            APPKIT_LOG_WARNING("DR16 remote offline due to data loss");
            self->lostCount_ = 0;
        }

        read_port.buffer_->Reset();

    continue_wrap:
        // 等待下一次读取
        appkit::osal::this_thread::SleepFor(3_ms); // 等待1/4帧数据时间
    }
}

#endif // ENABLED_MODULES_DR16