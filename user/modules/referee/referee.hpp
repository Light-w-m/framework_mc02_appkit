#pragma once

#include <referee_def.hpp>

#include <uart.hpp>
#include <osal_thread.hpp>
#include <osal_mutex.hpp>

#include <message.hpp>
#include <event.hpp>

#include <vector>
// #include <memory>

namespace referee
{
    /**
     * @brief 裁判系统类
     */
    class Referee final
    {
    public:
        /**
         * @brief 裁判系统状态枚举定义
         */
        enum class RefereeStatus : uint8_t
        {
            DISCONNECTED = 0, ///< 裁判系统未连接
            CONNECTED,        ///< 裁判系统已连接
        };

        /**
         * @brief 接收命令参数结构体
         */
        struct Param final
        {
            uint16_t cmd_id;        ///< 命令码 ID
            const char *topic_name; ///< 接收后发布的话题名称
            size_t type_size;       ///< 消息类型大小
        };

        /**
         * @brief 发送绑定器类
         */
        class SendBinder final
        {
            DECL_COPY_DISABLE(SendBinder)

        private:
            Referee &referee_;                 ///< 裁判系统引用
            appkit::TimePoint last_send_time_; ///< 上次发送时间点
            appkit::Duration send_interval_;   ///< 发送间隔

            appkit::osal::Mutex mutex_{}; ///< 互斥量
            appkit::WriteOperation op_{}; ///< 写入操作

        public:
            /**
             * @brief 构造函数
             * @param referee 裁判系统引用
             * @param send_interval 发送间隔
             */
            explicit SendBinder(Referee &referee, const appkit::Duration &send_interval)
                : referee_(referee), last_send_time_(appkit::Clock::system_clock->Now()), send_interval_(send_interval)
            {
            }

            /**
             * @brief 发送数据
             * @param data 发送数据
             */
            void SendData(const appkit::ConstRawData &data)
            {
                using namespace appkit;
                osal::LockGuard guard(mutex_);

                osal::this_thread::SleepUntil(last_send_time_, send_interval_);
                referee_.uart_.Write(data, op_);
            }
        };

        /**
         * @brief 构造函数
         * @param name 裁判系统名称
         * @param uart_name 串口设备名称
         * @param params 接收命令参数列表
         * @param read_buffer_size 读取缓冲区大小
         * @param read_thread_stack_size 读取线程栈大小
         */
        Referee(const char *name, const char *uart_name, std::initializer_list<Param> params, uint32_t read_buffer_size, uint32_t read_thread_stack_size = 1024);

        /**
         * @brief 析构函数
         */
        ~Referee();

    private:
        /**
         * @brief 裁判系统解析状态枚举定义
         */
        enum class RefereeParserState : uint8_t
        {
            WAIT_SOF = 0, ///< 等待帧起始标志
            WAIT_HEADER,  ///< 等待帧头
            WAIT_DATA,    ///< 等待数据区
            WAIT_CRC16,   ///< 等待 CRC16 校验码
        };

        /**
         * @brief 图传协议数据解析状态枚举定义
         */
        enum class VisionParserState : uint8_t
        {
            WAIT_SOF_1 = 0, ///< 等待帧起始标志 1
            WAIT_SOF_2,     ///< 等待帧起始标志 2
            WAIT_DATA,      ///< 等待数据区
            WAIT_CRC16,     ///< 等待 CRC16 校验码
        };

        /**
         * @brief 接收实例结构体
         */
        struct RecvInstance final
        {
            uint16_t cmd_id;     ///< 命令码 ID
            appkit::Topic topic; ///< 发布的话题
            size_t type_size;    ///< 消息类型大小
        };

        static constexpr appkit::Duration READ_THREAD_SLEEP_TIME = appkit::Duration::From<appkit::millisecond>(1); ///< 读取线程睡眠时间

        appkit::Uart uart_{};                                        ///< UART对象
        appkit::osal::Thread readThread_{};                          ///< 读取线程
        appkit::osal::Semaphore dataSem_{};                          ///< 数据就绪信号量
        appkit::ReadOperation op_{dataSem_, READ_THREAD_SLEEP_TIME}; ///< 读取操作 帧间隔1ms

        RefereeParserState parserState_{RefereeParserState::WAIT_SOF}; ///< 裁判系统解析状态
        RefereeHeader currentHeader_{};                                ///< 当前解析的帧头

        VisionParserState visionParserState_{VisionParserState::WAIT_SOF_1}; ///< 图传协议解析状态
        vision::VisionRemoteControl currentVisionData_{};                    ///< 当前解析的图传帧头

        std::vector<RecvInstance> recvInstances_{}; ///< 接收实例列表
        appkit::Topic visionTopic_{};               ///< 图传遥控器话题
        uint8_t *readBuffer_{};                     ///< 读取缓冲区

        appkit::Event refereeEvent_{}; ///< 裁判系统事件

        friend class SendBinder;

        /**
         * @brief 计算 CRC16 校验码
         * @param data_list 数据列表
         * @return CRC16 校验码
         */
        static uint16_t CalculateCRC16(std::initializer_list<appkit::ConstRawData> data_list);

        /**
         * @brief 解析裁判系统数据
         * @param available_size 可用数据大小
         */
        void ParseRefereeData(size_t available_size);

        /**
         * @brief 解析图传协议数据
         * @param available_size 可用数据大小
         */
        void ParseVisionData(size_t available_size);

        /**
         * @brief 读取线程函数
         * @param self Referee 对象指针
         */
        static void ReadThreadFunc(Referee *self);

        /**
         * @brief 发布消息
         * @param cmd_id 命令码 ID
         * @param data 消息数据
         */
        void PublishMessage(uint16_t cmd_id, appkit::ConstRawData data);
    };
} // namespace referee
