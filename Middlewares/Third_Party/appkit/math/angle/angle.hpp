#pragma once

#include <macros.hpp>
#include <numbers>

namespace appkit::math
{
    /**
     * @brief 角度单位命名空间
     */
    inline namespace angle_units
    {
        template <std::floating_point T>
        constexpr T DEGREE_TO_RADIAN = std::numbers::pi_v<T> / static_cast<T>(180); ///< 度转弧度转换系数

        template <std::floating_point T>
        constexpr T RADIAN_TO_DEGREE = static_cast<T>(180) / std::numbers::pi_v<T>; ///< 弧度转度转换系数
    } // namespace angle_units

    /**
     * @brief 角度类模板
     * @tparam T 浮点类型
     */
    template <std::floating_point T>
    class Angle
    {
    private:
        T radians_; ///< 角度值，单位：弧度

        /**
         * @brief 私有构造函数，使用弧度值初始化角度
         * @param radians 弧度值
         */
        constexpr Angle(const T radians) noexcept : radians_(radians) {}

    public:
        static constexpr Angle Zero{}; ///< 零角度常量

        static constexpr Angle Pi = Angle::FromRadians(std::numbers::pi_v<T>);            ///< π常量
        static constexpr Angle TwoPi = Angle::FromRadians(2 * std::numbers::pi_v<T>);     ///< 2π常量
        static constexpr Angle HalfPi = Angle::FromRadians(std::numbers::pi_v<T> / 2);    ///< π/2常量
        static constexpr Angle QuarterPi = Angle::FromRadians(std::numbers::pi_v<T> / 4); ///< π/4常量

        /**
         * @brief 构造函数，默认角度值为0弧度
         */
        FORCE_INLINE constexpr Angle() noexcept : radians_(static_cast<T>(0)) {}

        /**
         * @brief 构造函数，使用另一个角度对象初始化角度
         * @tparam U 另一个角度对象的浮点类型
         * @param other 另一个角度对象
         */
        template <std::floating_point U>
        FORCE_INLINE constexpr Angle(const Angle<U> &other) noexcept : radians_(static_cast<T>(other.radians_)) {}

        /**
         * @brief 构造函数，使用弧度值初始化角度
         * @param radians 弧度值
         */
        FORCE_INLINE static constexpr Angle FromRadians(T radians) noexcept
        {
            return Angle{radians};
        }

        /**
         * @brief 构造函数，使用度值初始化角度
         * @param degrees 度值
         */
        FORCE_INLINE static constexpr Angle FromDegrees(T degrees) noexcept
        {
            return Angle{degrees * DEGREE_TO_RADIAN<T>};
        }

        /**
         * @brief 获取角度的弧度值
         * @return 弧度值
         */
        [[nodiscard]] FORCE_INLINE constexpr T ToRadians() const noexcept
        {
            return radians_;
        }

        /**
         * @brief 获取角度的度值
         * @return 度值
         */
        [[nodiscard]] FORCE_INLINE constexpr T ToDegrees() const noexcept
        {
            return radians_ * RADIAN_TO_DEGREE<T>;
        }

        /**
         * @brief 限制角度在指定范围内
         * @param min 最小角度
         * @param max 最大角度
         * @return 限制后的角度
         */
        constexpr Angle Limit(const Angle &min, const Angle &max) const noexcept
        {
            const T range = max.radians_ - min.radians_;
            T adjusted_radians = radians_;

            while (adjusted_radians < min.radians_)
            {
                adjusted_radians += range;
            }

            while (adjusted_radians > max.radians_)
            {
                adjusted_radians -= range;
            }

            return Angle{adjusted_radians};
        }

        /**
         * @brief 归一化角度到[-π, π]范围内
         * @return 归一化后的角度
         */
        constexpr Angle Normalize() const noexcept
        {
            static constexpr Angle<T> pi = Angle<T>::FromRadians(std::numbers::pi_v<T>);

            return Limit(-pi, pi);
        }

        /**
         * @brief 重载加法运算符
         * @param other 另一个角度对象
         * @return 计算结果
         */
        FORCE_INLINE constexpr Angle operator+(const Angle &other) const noexcept
        {
            return Angle{radians_ + other.radians_};
        }

        /**
         * @brief 重载减法运算符
         * @param other 另一个角度对象
         * @return 计算结果
         */
        FORCE_INLINE constexpr Angle operator-(const Angle &other) const noexcept
        {
            return Angle{radians_ - other.radians_};
        }

        /**
         * @brief 重载取反运算符
         * @return 计算结果
         */
        FORCE_INLINE constexpr Angle operator-() const noexcept
        {
            return Angle{-radians_};
        }

        /**
         * @brief 重载乘法运算符
         * @param scalar 标量值
         * @return 计算结果
         */
        FORCE_INLINE constexpr Angle operator*(T scalar) const noexcept
        {
            return Angle{radians_ * scalar};
        }

        /**
         * @brief 重载除法运算符
         * @param scalar 标量值
         * @return 计算结果
         */
        FORCE_INLINE constexpr Angle operator/(T scalar) const noexcept
        {
            return Angle{radians_ / scalar};
        }

        /**
         * @brief 重载加法赋值运算符
         * @param other 另一个角度对象
         * @return 计算结果
         */
        FORCE_INLINE constexpr Angle &operator+=(const Angle &other) noexcept
        {
            radians_ += other.radians_;
            return *this;
        }

        /**
         * @brief 重载减法赋值运算符
         * @param other 另一个角度对象
         * @return 计算结果
         */
        FORCE_INLINE constexpr Angle &operator-=(const Angle &other) noexcept
        {
            radians_ -= other.radians_;
            return *this;
        }

        /**
         * @brief 重载乘法赋值运算符
         * @param scalar 标量值
         * @return 计算结果
         */
        FORCE_INLINE constexpr Angle &operator*=(T scalar) noexcept
        {
            radians_ *= scalar;
            return *this;
        }

        /**
         * @brief 重载除法赋值运算符
         * @param scalar 标量值
         * @return 计算结果
         */
        FORCE_INLINE constexpr Angle &operator/=(T scalar) noexcept
        {
            radians_ /= scalar;
            return *this;
        }

        /**
         * @brief 重载相等比较运算符
         * @param other 另一个角度对象
         * @return 计算结果
         */
        FORCE_INLINE constexpr bool operator==(const Angle &other) const noexcept
        {
            return radians_ == other.radians_;
        }

        /**
         * @brief 重载不等比较运算符
         * @param other 另一个角度对象
         * @return 计算结果
         */
        FORCE_INLINE constexpr bool operator!=(const Angle &other) const noexcept
        {
            return radians_ != other.radians_;
        }

        /**
         * @brief 重载小于比较运算符
         * @param other 另一个角度对象
         * @return 计算结果
         */
        FORCE_INLINE constexpr bool operator<(const Angle &other) const noexcept
        {
            return radians_ < other.radians_;
        }

        /**
         * @brief 重载小于等于比较运算符
         * @param other 另一个角度对象
         * @return 计算结果
         */
        FORCE_INLINE constexpr bool operator<=(const Angle &other) const noexcept
        {
            return radians_ <= other.radians_;
        }

        /**
         * @brief 重载大于比较运算符
         * @param other 另一个角度对象
         * @return 计算结果
         */
        FORCE_INLINE constexpr bool operator>(const Angle &other) const noexcept
        {
            return radians_ > other.radians_;
        }

        /**
         * @brief 重载大于等于比较运算符
         * @param other 另一个角度对象
         * @return 计算结果
         */
        FORCE_INLINE constexpr bool operator>=(const Angle &other) const noexcept
        {
            return radians_ >= other.radians_;
        }
    };

    /**
     * @brief 角度相关的用户自定义字面值后缀
     */
    namespace angle_literals
    {
        /**
         * @brief 角度后缀：弧度
         * @param value 数值
         * @return 角度对象
         */
        FORCE_INLINE consteval Angle<long double> operator"" _rad(long double value) noexcept
        {
            return Angle<long double>::FromRadians(value);
        }

        /**
         * @brief 角度后缀：度
         * @param value 数值
         * @return 角度对象
         */
        FORCE_INLINE consteval Angle<long double> operator"" _degree(long double value) noexcept
        {
            return Angle<long double>::FromDegrees(value);
        }
    } // namespace angle_literals

} // namespace appkit::math
