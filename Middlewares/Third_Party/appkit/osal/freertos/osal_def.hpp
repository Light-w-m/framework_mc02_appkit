/**
 * @file osal_def.h
 * @brief OSAL定义头文件
 * @author dusk
 */
#pragma once

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include <common_type.hpp>
#include <common_time.hpp>

namespace appkit::osal
{
    using ThreadId = TaskHandle_t;
    using SemaphoreId = SemaphoreHandle_t;
    using MutexId = SemaphoreHandle_t;
    using QueueId = QueueHandle_t;

    using SystemTimeUnit = std::ratio_divide<second, std::ratio<configTICK_RATE_HZ>>;

    /**
     * @brief 永远等待的时间间隔
     * @note 该值用于线程睡眠函数中，大于等于该值表示线程将被永久挂起，直到被外部事件唤醒
     * @note 该值应对应 FreeRTOS 的最大延时值 portMAX_DELAY 对应的时间间隔
     */
    inline constexpr auto gWaitForever{Duration::From<microsecond>(static_cast<uintmax_t>(portMAX_DELAY) * 1'000'000 / configTICK_RATE_HZ)};

    /**
     * @brief 检查当前是否在中断服务程序中
     * @return 如果在中断服务程序中则返回true，否则返回false
     */
    bool CheckInIsr() noexcept;
}