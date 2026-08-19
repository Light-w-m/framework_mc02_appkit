#pragma once

#include <driver.hpp>

namespace appkit
{
    /**
     * @brief Flash设备
     */
    class Flash final
    {
    public:
        /**
         * @brief Flash设备块基类
         */
        class Block : public Driver<Block>
        {
        public:
            uint32_t size;           ///< 大小，单位为字节
            uint32_t min_write_size; ///< 最小写大小，单位为字节
            uint32_t page_size;      ///< 页大小，单位为字节
            uint32_t sector_size;    ///< 扇区大小，单位为字节

            /**
             * @brief 擦除Flash数据
             * @param offset 偏移地址，单位为字节
             * @param size 擦除大小，单位为字节
             * @return 错误码
             */
            virtual ErrorCode Erase(uint32_t offset, uint32_t size) = 0;

            /**
             * @brief 写入Flash数据
             * @param offset 偏移地址，单位为字节
             * @param data 数据缓冲区
             * @return 错误码
             */
            virtual ErrorCode Write(uint32_t offset, ConstRawData data) = 0;

            /**
             * @brief 读取Flash数据
             * @param offset 偏移地址，单位为字节
             * @param data 数据缓冲区
             * @return 错误码
             */
            virtual ErrorCode Read(uint32_t offset, RawData data) = 0;
        };

        /**
         * @brief 构造函数
         */
        constexpr Flash()
        {
        }

        /**
         * @brief 以文件方式打开Flash
         * @param name 文件名称
         * @return 错误码
         */
        ErrorCode Open(const char *name);

        /**
         * @brief 读取Flash数据
         * @param offset 偏移地址，单位为字节
         * @param data 数据缓冲区
         * @return 错误码
         */
        ErrorCode Read(uint32_t offset, RawData data);

        /**
         * @brief 写入Flash数据
         * @param offset 偏移地址，单位为字节
         * @param data 数据缓冲区
         * @return 错误码
         */
        ErrorCode Write(uint32_t offset, ConstRawData data);

        /**
         * @brief 擦除Flash数据
         * @param offset 偏移地址，单位为字节
         * @param size 擦除大小，单位为字节
         * @return 错误码
         */
        ErrorCode Erase(uint32_t offset, uint32_t size);

        /**
         * @brief 获取Flash大小，单位为字节
         * @return Flash大小，单位为字节
         */
        uint32_t GetSize() const;

        /**
         * @brief 获取Flash最小写大小，单位为字节
         * @return Flash最小写大小，单位为字节
         */
        uint32_t GetMinWriteSize() const;

        /**
         * @brief 获取Flash页大小，单位为字节
         * @return Flash页大小，单位为字节
         */
        uint32_t GetPageSize() const;

        /**
         * @brief 获取Flash扇区大小，单位为字节
         * @return Flash扇区大小，单位为字节
         */
        uint32_t GetSectorSize() const;

    private:
        Block *block_{}; ///< 设备块指针
    };
}
