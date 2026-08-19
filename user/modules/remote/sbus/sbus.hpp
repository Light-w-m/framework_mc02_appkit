#pragma once

#ifdef ENABLED_MODULES_SBUS

#include <uart.hpp>
#include <osal_thread.hpp>

#include <message.hpp>
#include <event.hpp>

#include <remote.msg.hpp>

/**
 * @brief SBUS遥控器模块类
 */
class Sbus final
{
private:
#pragma pack(push, 1)
    /**
     * @brief SBUS数据包结构体
     * @note 首部（1字节）+ 数据（22字节）+ 标志位（1字节）+ 结束符（1字节） = 25字节
     *
     * 首部：起始字节 =0000 1111b （0x0f）
     * 数据：22 字节的数据，分别代表16个通道的数据，也即是每个通道的值用了 11 位来表示，22x8/16 = 11
     *     这样，每个通道的取值范围为 0~2047，低位在前、高位在后
     * 标志位：1字节，高四位从高到低依次表示：
     *     bit7：CH17数字通道
     *     bit6：CH16数字通道
     *     bit5：帧丢失(Frame lost)
     *     bit4：安全保护(Failsafe)：失控保护激活位(0x10）判断飞机是否失控
     *     bit3~bit0：低四位不用
     * 结束符：0x00
     */
    struct SbusPacket final
    {
        uint8_t start_byte;       ///< 起始字节，固定为0x0F
        int16_t channel_data[16]; ///< 通道数据
        struct
        {
            bool ch17 : 1;       ///< 通道17数字通道
            bool ch16 : 1;       ///< 通道16数字通道
            bool frame_lost : 1; ///< 帧丢失标志
            bool failsafe : 1;   ///< 失控保护标志
            uint8_t : 4;         ///< 保留位
        } flags;                 ///< 标志位
        uint8_t end_byte;        ///< 结束字节，固定为0x00或0x7E
    };

#pragma pack(pop)

    static constexpr uint8_t SBUS_START_BYTE = 0x0F;   ///< SBUS起始字节
    static constexpr uint8_t SBUS_END_BYTE_1 = 0x00;   ///< SBUS结束字节1
    static constexpr uint8_t SBUS_END_BYTE_2 = 0x7E;   ///< SBUS结束字节2
    static constexpr uint8_t SBUS_CHANNEL_COUNT = 16;  ///< SBUS通道数量
    static constexpr uint8_t SBUS_RAW_FRAME_SIZE = 25; ///< SBUS原始数据帧大小

    static constexpr uint16_t CHANNEL_MIDPOINT = SBUS_RANGE_MID; ///< 通道中点值
    static constexpr uint16_t CHANNEL_MAX = SBUS_RANGE_MAX;      ///< 通道最大值
    static constexpr uint16_t CHANNEL_MIN = SBUS_RANGE_MIN;      ///< 通道最小值

    static const appkit::Duration FRAME_PERIOD; ///< 帧间隔时间

    friend class SbusIndexChecker; ///< 友元类，用于通道索引检查

    appkit::Uart uart_{}; ///< UART对象
    SbusPacket packet_{}; ///< Sbus数据包

    uint32_t lostCount_{};                 ///< 丢包计数器
    control::RemoteCtrlData remoteData_{}; ///< 遥控器数据

    appkit::osal::Thread readThread_{};                ///< 读取线程
    appkit::osal::Semaphore dataSem_{};                ///< 数据就绪信号量
    appkit::ReadOperation op_{dataSem_, FRAME_PERIOD}; ///< 读取操作

    appkit::Event remoteEvent_{}; ///< 遥控器事件
    appkit::Topic remoteTopic_{}; ///< 遥控器数据主题

    /**
     * @brief 获取开关D档位值
     * @param channel_value 通道值
     * @return 档位值，上: 1 | 下: -1
     */
    static int8_t GetSwitchDValue(int16_t channel_value) noexcept;

    /**
     * @brief 获取开关T档位值
     * @param channel_value 通道值
     * @return 档位值，上: 1 | 中: 0 | 下: -1
     */
    static int8_t GetSwitchTValue(int16_t channel_value) noexcept;

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
     * @param self SBUS对象指针
     */
    static void ReadThreadFunc(Sbus *self);

public:
    /**
     * @brief 事件标志枚举定义
     */
    enum class EventFlag : uint8_t
    {
        OFFLINE,    ///< 离线
        FRAME_LOST, ///< 帧丢失
        FAILSAFE,   ///< 失控保护
        ONLINE,     ///< 在线
    };

    /**
     * @brief 构造函数
     * @param uart_name UART设备名称
     * @param read_thread_stack_size 读取线程栈大小
     */
    Sbus(const char *uart_name, uint32_t read_thread_stack_size = 512);
};

#endif // ENABLED_MODULES_SBUS