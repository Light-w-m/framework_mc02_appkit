#pragma once

#include <common_rw.hpp>
#include <driver.hpp>

namespace appkit
{
    /**
     * @brief SPI设备
     */
    class SPI final
    {
    public:
        /**
         * @brief SPI设备块基类
         */
        class Block : public Driver<Block>
        {
        public:
            /**
             * @brief 析构函数
             */
            virtual ~Block() = default;

            /**
             * @brief 读取数据
             * @param data 读取数据存放位置
             * @param operation 读取操作
             * @return 错误码
             */
            virtual ErrorCode Read(RawData data, ReadOperation &operation) = 0;

            /**
             * @brief 写入数据
             * @param data 写入数据位置
             * @param operation 写入操作
             * @return 错误码
             */
            virtual ErrorCode Write(ConstRawData data, WriteOperation &operation) = 0;

            /**
             * @brief 读写数据
             * @param tx_data 写入数据位置
             * @param rx_data 读取数据存放位置
             * @param operation 读写操作
             * @return 错误码
             */
            virtual ErrorCode ReadAndWrite(ConstRawData tx_data, RawData rx_data, ReadOperation &operation) = 0;
        };

        /**
         * @brief 构造函数
         */
        constexpr SPI() = default;

        /**
         * @brief SPI设备打开函数
         * @param name 设备名称
         * @return 错误码
         * @note 设备必须是RamFs中的文件，且文件类型必须是SPI::Block
         */
        ErrorCode Open(const char *name);

        /**
         * @brief 读取数据
         * @param data 读取数据存放位置
         * @param operation 读取操作
         * @return 错误码
         */
        ErrorCode Read(RawData data, ReadOperation &operation) const;

        /**
         * @brief 写入数据
         * @param data 写入数据位置
         * @param operation 写入操作
         * @return 错误码
         */
        ErrorCode Write(ConstRawData data, WriteOperation &operation) const;

        /**
         * @brief 读写数据
         * @param tx_data 写入数据位置
         * @param rx_data 读取数据存放位置
         * @param operation 读写操作
         * @return 错误码
         */
        ErrorCode ReadAndWrite(ConstRawData tx_data, RawData rx_data, ReadOperation &operation) const;

    private:
        Block *block_{};      ///< SPI设备接口指针
    };
} // namespace appkit
