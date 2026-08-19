#pragma once

#include <common_time.hpp>
#include <lockfree_list.hpp>

#include <driver.hpp>

namespace appkit
{
    /**
     * @brief 看门狗设备
     */
    class Watchdog final
    {
    private:
        /**
         * @brief 监视器结构体
         */
        struct Monitor final
        {
            Duration timeout_{};
            TimePoint next_feed_{};
        };

    public:
        /**
         * @brief 看门狗设备块基类
         */
        class Block : public Driver<Block>
        {
        protected:
            /**
             * @brief 更新监视器状态
             * @return 是否有监视器超时
             */
            bool UpdateMonitors();

        public:
            LockFreeList monitor_list_{}; ///< 监视器列表
            Duration auto_feed_time_{};   ///< 自动喂狗时间间隔
        };

        /**
         * @brief 构造函数
         */
        constexpr Watchdog() = default;

        /**
         * @brief 析构函数
         */
        ~Watchdog();

        /**
         * @brief 打开看门狗设备
         *
         * @param name 设备名称
         * @param timeout 实例超时时间
         * @return Errorcode
         */
        ErrorCode Open(const char *name, const Duration &timeout);

        /**
         * @brief 设置超时时间间隔
         * @param time 时间间隔
         */
        void SetTimeout(const Duration &time);

        /**
         * @brief 喂狗
         *
         */
        void Feed();

    private:
        Block *block_{nullptr};                      ///< 设备块指针
        LockFreeList::Node<Monitor> monitor_node_{}; ///< 监视器节点
    };
} // namespace appkit