#pragma once

#include <driver.hpp>
#include <common_rw.hpp>

namespace appkit
{
    class Uart final
    {
    public:
        /**
         * @brief 串口配置枚举定义
         */
        enum class Parity : uint8_t
        {
            NONE = 0, ///< 无奇偶校验
            EVEN,     ///< 偶校验
            ODD       ///< 奇校验
        };

        /**
         * @brief 停止位枚举定义
         */
        enum class StopBits : uint8_t
        {
            ONE = 0,        ///< 1位停止位
            ONE_POINT_FIVE, ///< 1.5位停止位
            TWO             ///< 2位停止位
        };

        /**
         * @brief 数据位枚举定义
         */
        enum class DataBits : uint8_t
        {
            FIVE = 5,  ///< 5位数据位
            SIX = 6,   ///< 6位数据位
            SEVEN = 7, ///< 7位数据位
            EIGHT = 8  ///< 8位数据位
        };

        /**
         * @brief 流控枚举定义
         */
        enum class FlowControl : uint8_t
        {
            NONE = 0, ///< 无流控
            RTS_CTS,  ///< 硬件流控
            XON_XOFF  ///< 软件流控
        };

        /**
         * @brief 波特率枚举定义
         */
        enum class BaudRate : uint32_t
        {
            BR_9600 = 9600,
            BR_19200 = 19200,
            BR_38400 = 38400,
            BR_57600 = 57600,
            BR_115200 = 115200,
            BR_230400 = 230400,
            BR_460800 = 460800,
            BR_921600 = 921600,
            BR_1000000 = 1000000,
            BR_2000000 = 2000000,
            BR_3000000 = 3000000,
            BR_4000000 = 4000000
        };

        /**
         * @brief 串口配置结构体
         */
        struct Config final
        {
            BaudRate baud_rate{BaudRate::BR_115200};     ///< 波特率
            DataBits data_bits{DataBits::EIGHT};         ///< 数据位
            Parity parity{Parity::NONE};                 ///< 奇偶校验
            StopBits stop_bits{StopBits::ONE};           ///< 停止位
            FlowControl flow_control{FlowControl::NONE}; ///< 流控
        };

        /**
         * @brief 串口设备接口
         */
        class Block : public Driver<Block>
        {
        public:
            WritePort *write{}; ///< 写接口
            ReadPort *read{};   ///< 读接口

            /**
             * @brief 析构函数
             */
            virtual ~Block() = default;

            /**
             * @brief 设置串口配置
             * @param config 配置参数
             * @return 错误码
             */
            virtual ErrorCode SetConfig(const Config &config)
            {
                (void)config;
                return ErrorCode::NOT_SUPPORTED;
            }
        };

        /**
         * @brief 构造函数
         */
        constexpr Uart() = default;

        /**
         * @brief 串口设备打开函数
         * @param name 设备名称
         * @return 错误码
         * @note 设备必须是RamFs中的文件，且文件类型必须是Uart::Block
         */
        ErrorCode Open(const char *name);

        /**
         * @brief 设置串口配置
         * @param config 配置参数
         * @return 错误码
         */
        ErrorCode SetConfig(const Config &config) const;

        /**
         * @brief 读取数据
         * @param data 读取数据存放位置
         * @param operation 读取操作
         * @return 错误码
         */
        ErrorCode Read(const RawData &data, ReadOperation &operation) const;

        /**
         * @brief 写入数据
         * @param data 写入数据位置
         * @param operation 写入操作
         * @return 错误码
         */
        ErrorCode Write(const ConstRawData &data, WriteOperation &operation) const;

        /**
         * @brief 获取写入端口
         * @return 写入端口指针
         */
        FORCE_INLINE WritePort *GetWritePort() const
        {
            return block_ ? block_->write : nullptr;
        }

        /**
         * @brief 获取读取端口
         * @return 读取端口指针
         */
        FORCE_INLINE ReadPort *GetReadPort() const
        {
            return block_ ? block_->read : nullptr;
        }

    private:
        Block *block_{};      ///< 串口设备接口指针
    };
}
