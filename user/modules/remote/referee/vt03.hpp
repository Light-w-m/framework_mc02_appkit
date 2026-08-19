#pragma once

#include <referee_def.hpp>

#include <message.hpp>
#include <event.hpp>

#include <remote.msg.hpp>
#include <keymouse.msg.hpp>

#include <osal_thread.hpp>

/**
 * @brief VT03遥控器模块类
 */
class VT03 final
{
private:
    appkit::Topic::Domain topic_domain_;                                   ///< 主题域
    appkit::Topic::SyncSuber<referee::vision::VisionRemoteControl> suber_; ///< 图传遥控器话题订阅者

    uint32_t lostCount_{};                     ///< 丢包计数器
    control::RemoteCtrlData remoteData_{};     ///< 遥控器数据
    control::KeymouseCtrlData keymouseData_{}; ///< 键鼠数据

    appkit::osal::Thread readThread_{}; ///< 读取线程

    appkit::Event remoteEvent_{};   ///< 遥控器事件
    appkit::Topic remoteTopic_{};   ///< 遥控器数据主题
    appkit::Topic keymouseTopic_{}; ///< 键鼠数据主题

    /**
     * @brief 读取线程函数
     * @param self VT03对象指针
     */
    static void ReadThreadFunc(VT03 *self) noexcept;

public:
    /**
     * @brief 事件标志枚举定义
     */
    enum class EventFlag : uint8_t
    {
        OFFLINE, ///< 离线
        ONLINE,  ///< 在线
    };

    /**
     * @brief 构造函数
     * @param referee_name 裁判系统名称
     * @param read_thread_stack_size 读取线程栈大小
     */
    explicit VT03(const char *referee_name, uint32_t read_thread_stack_size = 512);

    /**
     * @brief 析构函数
     */
    ~VT03() = default;
};