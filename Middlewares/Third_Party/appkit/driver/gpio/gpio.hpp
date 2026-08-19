#pragma once

#include <common_cb.hpp>
#include <lockfree_list.hpp>
#include <driver.hpp>

namespace appkit
{
    /**
     * @brief GPIO设备
     */
    class GPIO final
    {
    public:
        /**
         * @brief 回调函数类型
         */
        using Callback = appkit::Callback<>;

        /**
         * @brief GPIO设备块基类
         */
        class Block : public Driver<Block>
        {
        public:
            LockFreeList cb_list{}; ///< 回调列表

            /**
             * @brief 读取GPIO值
             * @return GPIO值
             */
            virtual bool Read() const = 0;

            /**
             * @brief 写入GPIO值
             * @param value 要写入的值
             * @return 错误码
             */
            virtual ErrorCode Write(bool value) = 0;

            /**
             * @brief 设置中断使能
             * @param enable 是否使能中断
             * @return 错误码
             */
            virtual ErrorCode SetInterrupt(bool enable) = 0;
        };

        /**
         * @brief 构造函数
         */
        constexpr GPIO() = default;

        /**
         * @brief 析构函数
         */
        ~GPIO();

        /**
         * @brief 打开GPIO设备
         * @param name 设备名称
         * @return 错误码
         */
        ErrorCode Open(const char *name);

        /**
         * @brief 读取GPIO值
         * @return GPIO值
         */
        bool Read() const;

        /**
         * @brief 写入GPIO值
         * @param value 要写入的值
         * @return 错误码
         */
        ErrorCode Write(bool value);

        /**
         * @brief 设置中断使能
         * @param enable 是否使能中断
         * @return 错误码
         */
        ErrorCode SetInterrupt(bool enable);

        /**
         * @brief 注册回调函数
         * @param cb 回调函数
         * @return 错误码
         */
        ErrorCode RegisterCallback(Callback cb);

        /**
         * @brief 比较两个GPIO对象是否相等
         * @param other 另一个GPIO对象
         * @return 是否相等
         */
        FORCE_INLINE bool operator==(const GPIO &other) const { return block_ == other.block_; }

        /**
         * @brief 检查GPIO对象是否已打开
         * @return 是否已打开
         */
        FORCE_INLINE operator bool() const { return block_ != nullptr; }

    private:
        bool is_register_{false};           ///< 是否已注册回调
        LockFreeList::Node<Callback> cb_{}; ///< 回调节点

        Block *block_{}; ///< GPIO设备块指针
    };
} // namespace appkit
