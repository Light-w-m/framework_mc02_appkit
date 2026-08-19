#pragma once

#include <osal_def.hpp>

namespace appkit::osal
{
    /**
     * @brief 信号量类
     */
    class Semaphore final
    {
    private:
        SemaphoreId handle_{};  ///< 信号量句柄

    public:
        /**
         * @brief 构造函数
         * @param initial_value 初始值
         */
        explicit Semaphore(uint32_t initial_value = 0);

        /**
         * @brief 析构函数
         */
        ~Semaphore();

        /**
         * @brief 发布信号量
         */
        void Post();

        /**
         * @brief 从中断或回调函数中发布信号量
         * @param in_isr 是否在中断中调用
         */
        void PostFromCallback(bool in_isr);

        /**
         * @brief 等待信号量
         * @param timeout 超时时间
         * @return 错误码
         */
        [[nodiscard]] ErrorCode Wait(const Duration& timeout);

        /**
         * @brief 获取信号量当前值
         * @return 信号量值
         */
        [[nodiscard]] uint32_t GetValue();
    };
} // namespace appkit::osal