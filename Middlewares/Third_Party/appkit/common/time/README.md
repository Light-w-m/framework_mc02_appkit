# Common Time

通用时间模块，提供了高精度时间间隔和时间点的获取、计算和存储功能

## 默认标量类型

时间存储的底层数据类型

```c++
using TimeScalar = intmax_t;
```

## 时间单位命名空间

提供部分时间单位定义以及时间单位约束定义

```c++
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
```

## 时间间隔 Duration

```c++
// 从字面量生成时间间隔
appkit::Duration dur = appkit::Duration::From<appkit::second>(1);

// 获取指定类型时间间隔
double second_dur = appkit::DurationCast<appkit::second, double>(dur);

// 时间间隔运算
appkit::Duration dur1, dur2;
float scale;

dur1 + dur2;
dur1 - dur2;
dur1 * scale;
dur2 / scale;

// 获取特殊时间间隔值
appkit::Duration max = appkit::Duration::Max();
appkit::Duration min = appkit::Duration::Min();
appkit::Duration zero = appkit::Duration::Zero();
```

## 时间点 TimePoint

```c++
// 从时间间隔生成时间点
appkit::TimePoint point(appkit::Duration::From<appkit::second>(1));

// 转换为时间间隔
appkit::Duration dur = point.SinceEpoch();

// 时间点运算
appkit::TimePoint point2 = point + dur;
appkit::TimePoint point3 = point - dur;

appkit::Duration dur2 = point2 - point3;

// 获取特殊时间点
appkit::TimePoint max = appkit::TimePoint::Max();
appkit::TimePoint min = appkit::TimePoint::Min();
appkit::TimePoint zero = appkit::TimePoint();
```

## 时钟基类 Clock

```c++
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
    static Clock *steady_clock; ///< 稳定时钟
    static Clock *system_clock; ///< 系统时钟

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
```

```c++
// 获取不同时钟时间
appkit::TimePoint system_timepoint = appkit::Clock::system_clock->Now();
appkit::TimePoint steady_timepoint = appkit::Clock::steady_clock->Now();

// 判断时钟是否为稳定时钟
bool is_system_clock_steady = appkit::Clock::system_clock->is_steady;
bool is_steady_clock_steady = appkit::Clock::steady_clock->is_steady;
```

## 时间单位转换 DurationCast

```c++
// 获取不同时钟单位时间间隔
double dur_ms = appkit::DurationCast<appkit::millsecond, double>(dur);
int dur_user = appkit::DurationCast<UserTimeUnit, int>(dur);
```

## 时间字面量

```c++
namespace time_literals
{
    /**
     * @brief 用户自定义字面量，纳秒
     * @param ns 纳秒值
     */
    inline consteval Duration operator""_ns(unsigned long long ns);

    /**
     * @brief 用户自定义字面量，微秒
     * @param us 微秒值
     */
    inline consteval Duration operator""_us(unsigned long long us);

    /**
     * @brief 用户自定义字面量，毫秒
     * @param ms 毫秒值
     */
    inline consteval Duration operator""_ms(unsigned long long ms);

    /**
     * @brief 用户自定义字面量，秒
     * @param s 秒值
     */
    inline consteval Duration operator""_s(unsigned long long s);

    /**
     * @brief 用户自定义字面量，分钟
     * @param min 分钟值
     */
    inline consteval Duration operator""_min(unsigned long long min);

    /**
     * @brief 用户自定义字面量，小时
     * @param h 小时值
     */
    inline consteval Duration operator""_h(unsigned long long h);

    /**
     * @brief 用户自定义字面量，天
     * @param d 天值
     */
    inline consteval Duration operator""_d(unsigned long long d);
} // namespace time_literals
```