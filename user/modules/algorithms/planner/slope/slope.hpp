#pragma once

#include <common_time.hpp>
#include <cmath>

namespace algorithm
{
    /**
     * @brief 斜坡限幅器类
     */
    class Slope final
    {
    private:
        const float maxIncAcc; ///< 最大加速度 单位 m/s²
        const float maxDecAcc; ///< 最大减速度 单位 m/s²

        /**
         * @brief 获取数值符号函数
         * @param value 输入数值
         * @return 符号：正数返回1，负数返回-1，零返回0
         */
        constexpr static ssize_t Sign(const float value) noexcept
        {
            if (value > 0.0f)
            {
                return 1;
            }
            else if (value < 0.0f)
            {
                return -1;
            }
            else
            {
                return 0;
            }
        }

    public:
        /**
         * @brief 构造函数
         * @param max_inc_acc 最大加速度 单位 m/s²
         * @param max_dec_acc 最大减速度 单位 m/s²
         */
        explicit constexpr Slope(float max_inc_acc, float max_dec_acc) noexcept
            : maxIncAcc(max_inc_acc), maxDecAcc(max_dec_acc)
        {
        }

        /**
         * @brief 计算给定初始速度和目标速度下的平滑速度
         * @param target_speed 目标速度 单位 m/s
         * @param current_speed 当前速度 单位 m/s
         * @param dt 时间间隔
         * @return 计算后的平滑速度 单位 m/s
         */
        constexpr float Update(const float target_speed, const float current_speed, const appkit::Duration &dt) const noexcept
        {
            const float dt_sec = appkit::DurationCast<appkit::second, float>(dt);
            
            // 计算速度差
            const float speed_diff = target_speed - current_speed;
            const float abs_speed_diff = std::abs(speed_diff);
            
            // 速度差接近零，直接返回目标速度
            if (abs_speed_diff < 1e-6f)
            {
                return target_speed;
            }

            // 根据速度差的符号选择加速度限制
            const ssize_t speed_diff_sign = Sign(speed_diff);
            const float acc_limit = (speed_diff_sign == Sign(current_speed)) ? maxIncAcc : maxDecAcc;

            // 计算允许的最大速度变化
            const float max_speed_change = acc_limit * dt_sec;

            // 限制速度变化
            float speed_change = speed_diff;
            if (abs_speed_diff > max_speed_change)
            {
                speed_change = speed_diff_sign * max_speed_change;
            }

            // 计算新的速度
            return current_speed + speed_change;
        }
    };
} // namespace algorithm
