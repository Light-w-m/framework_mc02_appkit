#include <vt03.hpp>

#include <logger.hpp>

using namespace appkit::time_literals;
using namespace referee::vision;

void VT03::ReadThreadFunc(VT03 *self) noexcept
{
    static constexpr float CHANNEL_SCALE = 2.0f / (VisionRemoteControl::CHANNEL_MAX - VisionRemoteControl::CHANNEL_MIN); ///< 通道缩放因子 [CHANNEL_MIN, CHANNEL_MAX] -> [-1, +1]

    size_t lost_count = 0, connected_count = 0;
    auto &remote_data = self->remoteData_.data;
    auto &keymouse_data = self->keymouseData_.data;

    while (true)
    {
        if (const auto ret = self->suber_.Wait(14_ms);
            Check(ret))
        {
            const auto &raw_data = self->suber_.Get();

            // 解析遥控器数据
            remote_data.rocker_r_ = (raw_data.channel_0 - VisionRemoteControl::CHANNEL_MIDPOINT) * CHANNEL_SCALE;
            remote_data.rocker_r1 = (raw_data.channel_1 - VisionRemoteControl::CHANNEL_MIDPOINT) * CHANNEL_SCALE;
            remote_data.rocker_l1 = (raw_data.channel_2 - VisionRemoteControl::CHANNEL_MIDPOINT) * CHANNEL_SCALE;
            remote_data.rocker_l_ = (raw_data.channel_3 - VisionRemoteControl::CHANNEL_MIDPOINT) * CHANNEL_SCALE;

            remote_data.switch_t1 = static_cast<int8_t>(raw_data.switch_mode) - 1; // C: -1 | N: 0 | S: +1
            remote_data.switch_d1 = static_cast<bool>(raw_data.switch_pause);
            remote_data.switch_d2 = static_cast<bool>(raw_data.switch_fn_l);
            remote_data.switch_d3 = static_cast<bool>(raw_data.switch_fn_r);
            remote_data.switch_d4 = static_cast<bool>(raw_data.trigger);

            remote_data.rocker_e1 = (raw_data.dial - VisionRemoteControl::CHANNEL_MIDPOINT) * CHANNEL_SCALE;

            keymouse_data.mouse_speed_x = raw_data.mouse_x / static_cast<float>(INT16_MAX);
            keymouse_data.mouse_speed_y = raw_data.mouse_y / static_cast<float>(INT16_MAX);
            keymouse_data.mouse_speed_z = raw_data.mouse_z / static_cast<float>(INT16_MAX);

            keymouse_data.press_l = static_cast<bool>(raw_data.mouse_l);
            keymouse_data.press_r = static_cast<bool>(raw_data.mouse_r);
            keymouse_data.press_m = static_cast<bool>(raw_data.mouse_m);

            keymouse_data.w = raw_data.key.w;
            keymouse_data.s = raw_data.key.s;
            keymouse_data.d = raw_data.key.d;
            keymouse_data.a = raw_data.key.a;
            keymouse_data.shift = raw_data.key.shift;
            keymouse_data.ctrl = raw_data.key.ctrl;
            keymouse_data.q = raw_data.key.q;
            keymouse_data.e = raw_data.key.e;
            keymouse_data.r = raw_data.key.r;
            keymouse_data.f = raw_data.key.f;
            keymouse_data.g = raw_data.key.g;
            keymouse_data.z = raw_data.key.z;
            keymouse_data.x = raw_data.key.x;
            keymouse_data.c = raw_data.key.c;
            keymouse_data.v = raw_data.key.v;
            keymouse_data.b = raw_data.key.b;

            // 发布数据
            self->remoteTopic_.Publish(self->remoteData_);
            self->keymouseTopic_.Publish(self->keymouseData_);

            // 连接状态处理
            lost_count = 0;
            if (++connected_count >= 100)
            {
                self->remoteEvent_.Activate(static_cast<uint32_t>(EventFlag::ONLINE));
                connected_count = 0;
            }
        }
        else
        {
            // 超时处理
            connected_count = 0;
            if (++lost_count >= 100)
            {
                self->remoteEvent_.Activate(static_cast<uint32_t>(EventFlag::OFFLINE));
                lost_count = 0;
            }
        }
    }
}

VT03::VT03(const char *referee_name, uint32_t read_thread_stack_size)
    : topic_domain_{referee_name}, suber_{"vt03_remote_control", &topic_domain_}
{
    // 创建遥控器数据主题
    appkit::Topic::Domain topic_domain{"control"};

    remoteTopic_ = appkit::Topic::CreateTopic<control::RemoteCtrlData>("remote_control", &topic_domain);
    keymouseTopic_ = appkit::Topic::CreateTopic<control::KeymouseCtrlData>("keymouse_control", &topic_domain);

    // 创建事件
    APPKIT_RAISE_IF_NOT(Check(remoteEvent_.Open("vt03")), "Failed to open VT03 remote event");

    // 创建读取线程
    readThread_.Create(ReadThreadFunc, this, "vt03_read_thread", read_thread_stack_size, appkit::osal::Thread::Priority::HIGH);
}