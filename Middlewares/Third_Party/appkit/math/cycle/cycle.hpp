#pragma once

#include <cmath>

namespace appkit::math
{
    /**
     * @brief 周期角度处理类
     * @tparam T 数值类型，支持浮点数类型如float、double等
     */
    template <typename T>
        requires std::is_floating_point_v<T>
    class CycleValue final
    {
    public:
        /**
         * @brief 构造函数
         * @param init_value 初始角度值
         */
        explicit CycleValue(T init_value = T{})
            : value_(init_value)
        {
        }

        /**
         * @brief 设置角度值
         * @param angle 角度值
         */
        void SetAngle(T angle)
        {
            value_ = angle;
        }

        /**
         * @brief 获取周期角度值
         * @param min 最小值
         * @param max 最大值
         * @return 周期角度值
         */
        T GetCycleAngle(T min, T max) const
        {
            T span = max - min;
            T adjusted_value = std::fmod(value_ - min, span);
            if (adjusted_value < min)
            {
                adjusted_value += span;
            }
            return adjusted_value + min;
        }

        /**
         * @brief 获取当前角度值
         * @return 当前角度值
         */
        T GetAngle() const
        {
            return value_;
        }

    private:
        T value_{}; ///< 当前角度值
    };
} // namespace appkit::math
