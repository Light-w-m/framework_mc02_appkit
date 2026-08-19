/**
 * @file common_time.h
 * @brief 通用时间定义
 * @author dusk
 * @date 2025-12-26
 */
#pragma once

#include <macros.hpp>

#include <numeric>
#include <ratio>
#include <cmath>

namespace appkit
{
    /**
     * @brief 默认标量类型
     */
    using TimeScalar = intmax_t;

    /**
     * @brief 时间单位命名空间
     */
    inline namespace time_units
    {
        using nanosecond = std::ratio<1>;                           ///< 纳秒
        using microsecond = std::ratio<1000>;                       ///< 微秒
        using millisecond = std::ratio<1000000>;                    ///< 毫秒
        using second = std::ratio<1000000000>;                      ///< 秒
        using minute = std::ratio_multiply<std::ratio<60>, second>; ///< 分钟
        using hour = std::ratio_multiply<std::ratio<60>, minute>;   ///< 小时
        using day = std::ratio_multiply<std::ratio<24>, hour>;      ///< 天

        /**
         * @brief 时间单位约束
         * @tparam T 类型
         */
        template <typename T>
        concept TimeUnitType = std::__is_ratio_v<T>;
    } // namespace time_units

    /**
     * @brief 时间间隔
     */
    class Duration final
    {
    private:
        TimeScalar duration_nano_{}; ///< 时间间隔，单位纳秒

        /**
         * @brief 私有构造函数
         * @param duration 时间间隔，单位纳秒
         */
        constexpr explicit Duration(TimeScalar duration) : duration_nano_(duration)
        {
        }

    public:
        /**
         * @brief 构造函数
         */
        constexpr Duration()
        {
        }

        /**
         * @brief 构造函数
         * @param other 时间间隔
         */
        constexpr Duration(const Duration &other) : duration_nano_{other.duration_nano_}
        {
        }

        /**
         * @brief 构造函数
         * @param other 时间间隔
         */
        constexpr Duration(Duration &&other) : duration_nano_{other.duration_nano_}
        {
        }

        /**
         * @brief 获取零时间间隔
         * @return 零时间间隔
         */
        FORCE_INLINE consteval static Duration Zero() { return Duration(0); }

        /**
         * @brief 获取最小时间间隔
         * @return 最小时间间隔
         */
        FORCE_INLINE consteval static Duration Min() { return Duration(std::numeric_limits<TimeScalar>::lowest()); }

        /**
         * @brief 获取最大时间间隔
         * @return 最大时间间隔
         */
        FORCE_INLINE consteval static Duration Max() { return Duration(std::numeric_limits<TimeScalar>::max()); }

        /**
         * @brief 赋值运算符
         * @param other 时间间隔
         */
        constexpr Duration &operator=(const Duration &other)
        {
            duration_nano_ = other.duration_nano_;
            return *this;
        }

        /**
         * @brief 赋值运算符
         * @param other 时间间隔
         */
        constexpr Duration &operator=(Duration &&other)
        {
            duration_nano_ = other.duration_nano_;
            return *this;
        }

        /**
         * @brief 从指定时间单位创建时间间隔
         * @tparam TimeUnit 时间单位，必须为std::ratio类型
         * @tparam Args 标量类型，必须为算术类型，默认Scalar
         * @param time 时间值
         * @return 时间间隔
         * @note 该函数将time转换为纳秒存储
         */
        template <TimeUnitType TimeUnit, std::convertible_to<TimeScalar> Args = TimeScalar>
        constexpr static Duration From(Args time)
        {
            return Duration(time * TimeUnit::num / TimeUnit::den);
        }

        /**
         * @brief 转换为指定时间单位的时间值
         * @tparam TimeUnit 时间单位，必须为std::ratio类型
         * @tparam TargetScalar 目标标量类型，必须为算术类型，默认Scalar
         * @return 时间值
         * @note 该函数将存储的纳秒转换为指定时间单位的时间值
         */
        template <TimeUnitType TimeUnit, std::convertible_to<TimeScalar> TargetScalar = TimeScalar>
        constexpr FORCE_INLINE TargetScalar To() const
        {
            // 浮点数直接转换
            if constexpr (std::is_floating_point_v<TargetScalar>)
            {
                return static_cast<TargetScalar>(duration_nano_) * TimeUnit::den / TimeUnit::num;
            }

            std::common_type_t<TimeScalar, TargetScalar> ret = duration_nano_ * TimeUnit::den / TimeUnit::num;

            // 溢出检查 最大值
            if constexpr (std::numeric_limits<TargetScalar>::max() < std::numeric_limits<TimeScalar>::max())
            {
                if (ret > static_cast<TimeScalar>(std::numeric_limits<TargetScalar>::max()))
                {
                    return std::numeric_limits<TargetScalar>::max();
                }
            }

            // 溢出检查 最小值
            if constexpr (std::numeric_limits<TargetScalar>::lowest() > std::numeric_limits<TimeScalar>::lowest())
            {
                if (ret < static_cast<TimeScalar>(std::numeric_limits<TargetScalar>::lowest()))
                {
                    return std::numeric_limits<TargetScalar>::lowest();
                }
            }

            return ret;
        }

        /**
         * @brief 减法运算符
         * @param other 另一个时间间隔
         * @return 结果时间间隔
         */
        constexpr Duration operator-(const Duration &other) const
        {
            if (duration_nano_ < 0 && other.duration_nano_ < 0 && duration_nano_ - other.duration_nano_ > 0)
            {
                return Duration::Min();
            }
            return Duration(duration_nano_ - other.duration_nano_);
        }

        /**
         * @brief 加法运算符
         * @param other 另一个时间间隔
         * @return 结果时间间隔
         */
        constexpr Duration operator+(const Duration &other) const
        {
            if (duration_nano_ > 0 && other.duration_nano_ > 0 && duration_nano_ + other.duration_nano_ < 0)
            {
                return Duration::Max();
            }
            return Duration(duration_nano_ + other.duration_nano_);
        }

        /**
         * @brief 加法赋值运算符
         * @param other 另一个时间间隔
         * @return 当前时间间隔引用
         */
        constexpr Duration &operator+=(const Duration &other)
        {
            if (duration_nano_ > 0 && other.duration_nano_ > 0 && duration_nano_ + other.duration_nano_ < 0)
            {
                duration_nano_ = std::numeric_limits<TimeScalar>::max();
            }
            else
            {
                duration_nano_ += other.duration_nano_;
            }
            return *this;
        }

        /**
         * @brief 减法赋值运算符
         * @param other 另一个时间间隔
         * @return 当前时间间隔引用
         */
        constexpr Duration &operator-=(const Duration &other)
        {
            if (duration_nano_ < 0 && other.duration_nano_ < 0 && duration_nano_ - other.duration_nano_ > 0)
            {
                duration_nano_ = std::numeric_limits<TimeScalar>::lowest();
            }
            else
            {
                duration_nano_ -= other.duration_nano_;
            }
            return *this;
        }

        /**
         * @brief 乘法运算符
         * @param scalar 标量
         * @return 结果时间间隔
         */
        constexpr FORCE_INLINE Duration operator*(auto scalar) const
        {
            return Duration(duration_nano_ * scalar);
        }

        /**
         * @brief 除法运算符
         * @param scalar 标量
         * @return 结果时间间隔
         */
        constexpr FORCE_INLINE Duration operator/(auto scalar) const
        {
            return Duration(duration_nano_ / scalar);
        }

        /**
         * @brief 乘法赋值运算符
         * @param scalar 标量
         * @return 当前时间间隔引用
         */
        constexpr Duration &operator*=(auto scalar)
        {
            duration_nano_ *= scalar;
            return *this;
        }

        /**
         * @brief 除法赋值运算符
         * @param scalar 标量
         * @return 当前时间间隔引用
         */
        constexpr Duration &operator/=(auto scalar)
        {
            duration_nano_ /= scalar;
            return *this;
        }

        /**
         * @brief 三路比较运算符
         * @param other 另一个时间间隔
         * @return 比较结果
         */
        constexpr FORCE_INLINE auto operator<=>(const Duration &other) const
        {
            return duration_nano_ <=> other.duration_nano_;
        }

        /**
         * @brief 比较运算符 不等于
         * @param other 另一个时间间隔
         * @return 比较结果
         */
        constexpr FORCE_INLINE bool operator==(const Duration &other) const = default;
    };

    /**
     * @brief 时间点
     */
    class TimePoint final
    {
    private:
        Duration time_{Duration::Zero()}; ///< 时间点，表示自时间戳起点以来的时间间隔

    public:
        /**
         * @brief 构造函数
         */
        constexpr TimePoint()
        {
        }

        /**
         * @brief 构造函数
         * @param _dur 时间间隔
         */
        constexpr explicit TimePoint(const Duration &_dur) : time_(_dur)
        {
        }

        /**
         * @brief 获取最小时间点
         * @return 最小时间点
         */
        FORCE_INLINE static consteval TimePoint Min()
        {
            return TimePoint{Duration::Min()};
        }

        /**
         * @brief 获取最大时间点
         * @return 最大时间点
         */
        FORCE_INLINE static consteval TimePoint Max()
        {
            return TimePoint{Duration::Max()};
        }

        /**
         * @brief 获取自时间戳起点以来的时间间隔
         * @return 时间间隔;
         */
        constexpr FORCE_INLINE Duration SinceEpoch() const
        {
            return time_;
        }

        /**
         * @brief 加法赋值运算符
         * @param _dur 时间间隔
         * @return 当前时间点引用
         */
        constexpr TimePoint &operator+=(const Duration &_dur)
        {
            time_ += _dur;
            return *this;
        }

        /**
         * @brief 减法赋值运算符
         * @param _dur 时间间隔
         * @return 当前时间点引用
         */
        constexpr TimePoint &operator-=(const Duration &_dur)
        {
            time_ -= _dur;
            return *this;
        }

        /**
         * @brief 加法运算符
         * @param _dur 时间间隔
         * @return 结果时间点
         */
        constexpr FORCE_INLINE TimePoint operator+(const Duration &_dur) const
        {
            return TimePoint{Duration(time_ + _dur)};
        }

        /**
         * @brief 减法运算符
         * @param _dur 时间间隔
         * @return 结果时间点
         */
        constexpr FORCE_INLINE TimePoint operator-(const Duration &_dur) const
        {
            return TimePoint{Duration(time_ - _dur)};
        }

        /**
         * @brief 减法运算符
         * @param _dur 另一个时间点
         * @return 结果时间间隔
         */
        constexpr FORCE_INLINE Duration operator-(const TimePoint &_dur) const
        {
            return Duration(time_ - _dur.time_);
        }

        /**
         * @brief 三路比较运算符
         * @param _dur 另一个时间点
         * @return 比较结果
         */
        constexpr FORCE_INLINE auto operator<=>(const TimePoint &_dur) const
        {
            return time_ <=> _dur.time_;
        }

        /**
         * @brief 比较运算符 等于
         * @param _dur 另一个时间点
         * @return 比较结果
         */
        constexpr FORCE_INLINE bool operator==(const TimePoint &_dur) const = default;
    };

    /**
     * @brief 时钟基类
     */
    class Clock
    {
    protected:
        /**
         * @brief 构造函数
         * @param steady 是否为稳定时钟
         */
        constexpr Clock(bool steady) : is_steady(steady)
        {
        }

    public:
        static const Clock *steady_clock; ///< 稳定时钟
        static const Clock *system_clock; ///< 系统时钟

        const bool is_steady; ///< 是否为稳定时钟

        /**
         * @brief 析构函数
         */
        virtual ~Clock() = default;

        /**
         * @brief 获取当前时间点
         * @return 当前时间点
         */
        virtual TimePoint Now() const = 0;
    };

    /**
     * @brief 时间单位转换
     * @tparam TimeUnit 目标时间单位，必须为std::ratio类型
     * @tparam Scalar 目标标量类型，必须为算术类型
     * @param time 时间间隔
     * @return 转换后的时间值
     * @note 该函数将time转换为目标时间单位的时间值
     */
    template <TimeUnitType TimeUnit, std::convertible_to<TimeScalar> Scalar = TimeScalar>
    FORCE_INLINE constexpr Scalar DurationCast(const Duration &time)
    {
        return time.template To<TimeUnit, Scalar>();
    }

    /**
     * @brief 时间字面量命名空间
     */
    namespace time_literals
    {
        /**
         * @brief 用户自定义字面量，纳秒
         * @param ns 纳秒值
         */
        FORCE_INLINE consteval Duration operator""_ns(unsigned long long ns)
        {
            return Duration::From<nanosecond>(ns);
        }

        /**
         * @brief 用户自定义字面量，微秒
         * @param us 微秒值
         */
        FORCE_INLINE consteval Duration operator""_us(unsigned long long us)
        {
            return Duration::From<microsecond>(us);
        }

        /**
         * @brief 用户自定义字面量，毫秒
         * @param ms 毫秒值
         */
        FORCE_INLINE consteval Duration operator""_ms(unsigned long long ms)
        {
            return Duration::From<millisecond>(ms);
        }

        /**
         * @brief 用户自定义字面量，秒
         * @param s 秒值
         */
        FORCE_INLINE consteval Duration operator""_s(unsigned long long s)
        {
            return Duration::From<second>(s);
        }

        /**
         * @brief 用户自定义字面量，分钟
         * @param min 分钟值
         */
        FORCE_INLINE consteval Duration operator""_min(unsigned long long min)
        {
            return Duration::From<minute>(min);
        }

        /**
         * @brief 用户自定义字面量，小时
         * @param h 小时值
         */
        FORCE_INLINE consteval Duration operator""_h(unsigned long long h)
        {
            return Duration::From<hour>(h);
        }

        /**
         * @brief 用户自定义字面量，天
         * @param d 天值
         */
        FORCE_INLINE consteval Duration operator""_d(unsigned long long d)
        {
            return Duration::From<day>(d);
        }
    } // namespace time_literals
} // namespace appkit
