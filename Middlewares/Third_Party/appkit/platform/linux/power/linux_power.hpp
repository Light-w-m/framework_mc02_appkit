#pragma once

#include <power.hpp>
#include <event.hpp>

namespace appkit
{
    /**
     * @brief Linux平台电源管理设备
     */
    class LinuxPower final : public PowerManager::Block
    {
    public:
        /**
         * @brief 构造函数
         */
        LinuxPower();

        /**
         * @brief 改变电源状态
         * @param status 新状态
         */
        virtual void ChangeStatus(PowerManager::Block::Status status);

    private:
        Event reset_event_;     ///< 重置信号量
        Event shutdown_event_;  ///< 关机信号量

        /**
         * @brief 检查是否有root权限
         */
        static void CheckRoot();

        /**
         * @brief 重置系统
         */
        static void Reset();

        /**
         * @brief 关闭系统
         */
        static void Shutdown();
    };
} // namespace appkit