#pragma once

#ifdef ENABLED_MODULES_DR16

#include <uart.hpp>
#include <osal_thread.hpp>

#include <message.hpp>
#include <event.hpp>

#include <remote.msg.hpp>
#include <keymouse.msg.hpp>

/**
 * @brief DJI DR16遥控器模块类
 */
class DR16 final
{
private:
#pragma pack(push, 1)
    /**
     * @brief DJI DR16数据包结构体
     */
    struct DR16Packet final
    {
        uint16_t channel_0 : 11; ///< 左水平摇杆
        uint16_t channel_1 : 11; ///< 左垂直摇杆
        uint16_t channel_2 : 11; ///< 右水平摇杆
        uint16_t channel_3 : 11; ///< 右垂直摇杆

        uint8_t s1 : 2; ///< 左侧开关
        uint8_t s2 : 2; ///< 右侧开关

        int16_t mouse_x; ///< 鼠标X轴移动速度
        int16_t mouse_y; ///< 鼠标Y轴移动速度
        int16_t mouse_z; ///< 鼠标滚轮移动速度

        uint8_t press_l; ///< 鼠标左键
        uint8_t press_r; ///< 鼠标右键

        struct
        {
            bool w : 1;     ///< W键
            bool s : 1;     ///< S键
            bool d : 1;     ///< D键
            bool a : 1;     ///< A键
            bool shift : 1; ///< Shift键
            bool ctrl : 1;  ///< Ctrl键
            bool q : 1;     ///< Q键
            bool e : 1;     ///< E键
            bool r : 1;     ///< R键
            bool f : 1;     ///< F键
            bool g : 1;     ///< G键
            bool z : 1;     ///< Z键
            bool x : 1;     ///< X键
            bool c : 1;     ///< C键
            bool v : 1;     ///< V键
            bool b : 1;     ///< B键
        } key;

        uint16_t dial; ///< 附加拨轮
    };
#pragma pack(pop)

    static constexpr uint16_t CHANNEL_MIDPOINT = 1024; ///< 通道中点值
    static constexpr uint16_t CHANNEL_MAX = 1684;      ///< 通道最大值
    static constexpr uint16_t CHANNEL_MIN = 364;       ///< 通道最小值

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
     * @param uart_name UART设备名称
     * @param read_thread_stack_size 读取线程栈大小
     */
    DR16(const char *uart_name, uint32_t read_thread_stack_size = 512);

private:
    appkit::Uart uart_{};  ///< UART对象
    DR16Packet rawData_{}; ///< 原始数据包

    uint32_t lostCount_{}; ///< 丢包计数器

    control::RemoteCtrlData remoteData_{};     ///< 遥控器数据
    control::KeymouseCtrlData keymouseData_{}; ///< 键鼠数据

    appkit::osal::Thread readThread_{};                                                   ///< 读取线程
    appkit::osal::Semaphore dataSem_{};                                                   ///< 数据就绪信号量
    appkit::ReadOperation op_{dataSem_, appkit::Duration::From<appkit::millisecond>(20)}; ///< 读取操作 帧间隔14ms

    appkit::Event remoteEvent_{};   ///< 遥控器事件
    appkit::Topic remoteTopic_{};   ///< 遥控器数据主题
    appkit::Topic keymouseTopic_{}; ///< 键鼠数据主题

    /**
     * @brief 检查通道数据有效性
     * @param data 遥控器数据
     * @return 是否有效
     */
    bool CheckChannelValid() const noexcept;

    /**
     * @brief 解析数据包
     */
    void ParseData() noexcept;

    /**
     * @brief 读取线程函数
     * @param self DR16对象指针
     */
    static void ReadThreadFunc(DR16 *self);
};

#endif // ENABLED_MODULES_DR16