#pragma once

#include <common_type.hpp>
#include <common_time.hpp>

namespace appkit::osal
{
    /**
     * @brief 初始化OSAL子系统
     * @return 错误码
     */
    ErrorCode Init();

    /**
     * @brief 设置系统时间
     * @param time_point 时间点
     * @return 错误码
     */
    ErrorCode SetTime(const TimePoint &time_point);
} // namespace appkit::osal