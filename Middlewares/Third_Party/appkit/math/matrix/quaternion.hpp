#pragma once

#include <utility>
#include <matrix.hpp>

namespace appkit::math
{
    /**
     * @brief 四元数类
     * @tparam T 数值类型
     */
    template <std::floating_point T>
    class Quaternion final
    {
    private:
        Vector<T, 4> data_; ///< 四元数数据，按顺序存储为 (w, x, y, z)

    public:
        /**
         * @brief 默认构造函数，初始化为单位四元数
         */
        FORCE_INLINE constexpr Quaternion() noexcept
            : data_{1, 0, 0, 0}
        {
        }

        /**
         * @brief 构造函数，使用给定分量初始化四元数
         * @param w 实部
         * @param x 虚部x分量
         * @param y 虚部y分量
         * @param z 虚部z分量
         */
        constexpr Quaternion(T w, T x, T y, T z) noexcept
            : data_{w, x, y, z}
        {
        }

        /**
         * @brief 构造函数，使用给定向量初始化四元数
         * @param vector 包含四元数分量的向量，按顺序为 (w, x, y, z)
         */
        FORCE_INLINE constexpr Quaternion(const Vector<T, 4> &vector) noexcept
            : data_{vector}
        {
        }

        /**
         * @brief 复制赋值运算符
         * @param other 其他四元数
         * @return 当前四元数引用
         */
        FORCE_INLINE constexpr Quaternion<T> &operator=(const Quaternion<T> &other) noexcept
        {
            data_ = other.data_;
            return *this;
        }

        /**
         * @brief 从滚转-俯仰-偏航角创建四元数
         * @param roll 滚转角，单位：弧度
         * @param pitch 俯仰角，单位：弧度
         * @param yaw 偏航角，单位：弧度
         * @return 生成的四元数
         */
        constexpr static Quaternion<T> FromRPY(const T roll, const T pitch, const T yaw) noexcept
        {
            const T half_roll = roll * 0.5;
            const T half_pitch = pitch * 0.5;
            const T half_yaw = yaw * 0.5;

            const T cy = std::cos(half_yaw);
            const T sy = std::sin(half_yaw);
            const T cr = std::cos(half_roll);
            const T sr = std::sin(half_roll);
            const T cp = std::cos(half_pitch);
            const T sp = std::sin(half_pitch);

            return Quaternion<T>(
                cy * cr * cp + sy * sr * sp, // w
                cy * sr * cp - sy * cr * sp, // x
                cy * cr * sp + sy * sr * cp, // y
                sy * cr * cp - cy * sr * sp  // z
            );
        }

        /**
         * @brief 从滚转-俯仰-偏航角创建四元数（别名）
         * @param roll 滚转角，单位：弧度
         * @param pitch 俯仰角，单位：弧度
         * @param yaw 偏航角，单位：弧度
         * @return 生成的四元数
         */
        FORCE_INLINE constexpr static Quaternion<T> FromEuler(const T roll, const T pitch, const T yaw) noexcept
        {
            return FromRPY(roll, pitch, yaw);
        }

        /**
         * @brief 获取滚转-俯仰-偏航角表示
         * @param roll 输出滚转角，单位：弧度
         * @param pitch 输出俯仰角，单位：弧度
         * @param yaw 输出偏航角，单位：弧度
         */
        constexpr void GetRPY(T &roll, T &pitch, T &yaw) const noexcept
        {
            const auto &[w, x, y, z] = data_;

            if (const auto near_singularity = w * y - z * x;
                std::abs(near_singularity) >= 0.5 - std::numeric_limits<T>::epsilon())
            {
                int sign = (near_singularity > 0) ? 1 : -1;

                roll = 0;
                pitch = sign * (0.5 * std::numbers::pi_v<T>);
                yaw = -2 * sign * std::atan2(x, w);
            }
            else
            {
                const auto ww = w * w, xx = x * x, zz = z * z;

                roll = std::atan2(2 * (w * x + y * z), 2 * (ww + zz) - 1);
                pitch = std::asin(2 * near_singularity);
                yaw = std::atan2(2 * (w * z + x * y), 2 * (ww + xx) - 1);
            }

            // // 计算滚转角 (x轴旋转)
            // const T sinr_cosp = static_cast<T>(2) * (w * x + y * z);
            // const T cosr_cosp = static_cast<T>(1) - static_cast<T>(2) * (x * x + y * y);
            // roll = std::atan2(sinr_cosp, cosr_cosp);

            // // 计算俯仰角 (y轴旋转)
            // const T sinp = static_cast<T>(2) * (w * y - z * x);
            // if (std::abs(sinp) >= static_cast<T>(1))
            // {
            //     pitch = std::copysign(static_cast<T>(M_PI) / static_cast<T>(2), sinp); // 使用90度
            // }
            // else
            // {
            //     pitch = std::asin(sinp);
            // }

            // // 计算偏航角 (z轴旋转)
            // const T siny_cosp = static_cast<T>(2) * (w * z + x * y);
            // const T cosy_cosp = static_cast<T>(1) - static_cast<T>(2) * (y * y + z * z);
            // yaw = std::atan2(siny_cosp, cosy_cosp);
        }

        /**
         * @brief 获取滚转-俯仰-偏航角表示（别名）
         * @param roll 输出滚转角，单位：弧度
         * @param pitch 输出俯仰角，单位：弧度
         * @param yaw 输出偏航角，单位：弧度
         */
        FORCE_INLINE constexpr void GetEuler(T &roll, T &pitch, T &yaw) const noexcept
        {
            GetRPY(roll, pitch, yaw);
        }

        /**
         * @brief 访问四元数W分量
         * @return W分量引用
         */
        FORCE_INLINE constexpr T &W() noexcept
        {
            return data_(0, 0);
        }

        /**
         * @brief 访问四元数X分量
         * @return X分量引用
         */
        FORCE_INLINE constexpr T &X() noexcept
        {
            return data_(1, 0);
        }

        /**
         * @brief 访问四元数Y分量
         * @return Y分量引用
         */
        FORCE_INLINE constexpr T &Y() noexcept
        {
            return data_(2, 0);
        }

        /**
         * @brief 访问四元数Z分量
         * @return Z分量引用
         */
        FORCE_INLINE constexpr T &Z() noexcept
        {
            return data_(3, 0);
        }

        /**
         * @brief 访问四元数W分量常量
         * @return W分量常量引用
         */
        FORCE_INLINE constexpr const T &W() const noexcept
        {
            return data_(0, 0);
        }

        /**
         * @brief 访问四元数X分量常量
         * @return X分量常量引用
         */
        FORCE_INLINE constexpr const T &X() const noexcept
        {
            return data_(1, 0);
        }

        /**
         * @brief 访问四元数Y分量常量
         * @return Y分量常量引用
         */
        FORCE_INLINE constexpr const T &Y() const noexcept
        {
            return data_(2, 0);
        }

        /**
         * @brief 访问四元数Z分量常量
         * @return Z分量常量引用
         */
        FORCE_INLINE constexpr const T &Z() const noexcept
        {
            return data_(3, 0);
        }

        /**
         * @brief 生成共轭四元数
         * @return 共轭四元数
         */
        FORCE_INLINE constexpr Quaternion<T> Conjugate() const noexcept
        {
            const auto &[w, x, y, z] = data_;

            return Quaternion<T>(w, -x, -y, -z);
        }

        /**
         * @brief 计算四元数的范数
         * @return 范数值
         */
        FORCE_INLINE constexpr T Norm() const noexcept
        {
            return norm(data_);
        }

        /**
         * @brief 获取归一化后的四元数
         * @return 归一化后的四元数
         */
        FORCE_INLINE constexpr Quaternion<T> Normalized() const noexcept
        {
            return Quaternion<T>(data_ / norm(data_));
        }

        /**
         * @brief 将四元数归一化
         */
        FORCE_INLINE constexpr void Normalize() noexcept
        {
            data_ /= norm(data_);
        }

        /**
         * @brief 将四元数转换为旋转矩阵
         * @return 3x3旋转矩阵
         */
        constexpr Matrix<T, 3, 3> ToRotationMatrix() const noexcept
        {
            const auto &[w, x, y, z] = data_;

            const auto xx = x * x,
                       yy = y * y,
                       zz = z * z,
                       wx = w * x,
                       wy = w * y,
                       wz = w * z,
                       xy = x * y,
                       xz = x * z,
                       yz = y * z;

            return Matrix<T, 3, 3>{
                (1 - 2 * (yy + zz)), (2 * (xy - wz)), (2 * (xz + wy)),
                (2 * (xy + wz)), (1 - 2 * (xx + zz)), (2 * (yz - wx)),
                (2 * (xz - wy)), (2 * (yz + wx)), (1 - 2 * (xx + yy))};
        }

        /**
         * @brief 四元数加法运算符
         * @param other 其他四元数
         * @return 和四元数
         */
        FORCE_INLINE constexpr Quaternion<T> operator+(Quaternion<T> other) const noexcept
        {
            return Quaternion<T>(data_ + other.data_);
        }

        /**
         * @brief 四元数乘法运算符
         * @param other 其他四元数
         * @return 乘积四元数
         */
        FORCE_INLINE constexpr Quaternion<T> operator*(const Quaternion<T> &other) const noexcept
        {
            const auto &[w, x, y, z] = data_;
            const auto &[ow, ox, oy, oz] = other;

            return Quaternion<T>(
                w * ow - x * ox - y * oy - z * oz,
                w * ox + x * ow + y * oz - z * oy,
                w * oy - x * oz + y * ow + z * ox,
                w * oz + x * oy - y * ox + z * ow);
        }

        /**
         * @brief 四元数乘法赋值运算符
         * @param other 其他四元数
         * @return 当前四元数引用
         */
        FORCE_INLINE constexpr Quaternion<T> &operator*=(const Quaternion<T> &other) noexcept
        {
            const auto [w, x, y, z] = data_;
            const auto &[ow, ox, oy, oz] = other;

            data_(0, 0) = w * ow - x * ox - y * oy - z * oz;
            data_(1, 0) = w * ox + x * ow + y * oz - z * oy;
            data_(2, 0) = w * oy - x * oz + y * ow + z * ox;
            data_(3, 0) = w * oz + x * oy - y * ox + z * ow;

            return *this;
        }

        /**
         * @brief 获取指定索引的分量
         * @tparam Index 分量索引（0: w, 1: x, 2: y, 3: z）
         * @return 分量的引用
         *
         * @note 仅用于支持结构化绑定
         */
        template <size_t Index>
        FORCE_INLINE constexpr auto &get(this auto &&self) noexcept
        {
            static_assert(Index < std::tuple_size<Quaternion<T>>::value, "Index out of bounds for Quaternion");
            return self.data_(Index, 0);
        }
    };

    /**
     * @brief 单精度四元数别名
     */
    using Quaternionf = Quaternion<float>;

    /**
     * @brief 双精度四元数别名
     */
    using Quaterniond = Quaternion<double>;
}

namespace std
{
    /**
     * @brief 为Quaternion特化std::tuple_size
     * @note 仅用于支持结构化绑定
     */
    template <typename T>
    struct tuple_size<appkit::math::Quaternion<T>>
    {
        static constexpr inline size_t value = 4;
    };

    /**
     * @brief 为Quaternion特化std::tuple_element
     * @note 仅用于支持结构化绑定
     */
    template <size_t Index, typename T>
    struct tuple_element<Index, appkit::math::Quaternion<T>>
    {
        static_assert(Index < tuple_size<appkit::math::Quaternion<T>>::value, "Index out of bounds for Quaternion");
        using type = T;
    };
} // namespace std