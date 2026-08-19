#pragma once

#include <common_time.hpp>

#include <matrix.hpp>
#include <quaternion.hpp>
// #include <Eigen/Core>
// // #include <Eigen/Geometry>

#include <algorithm>
#include <cmath>

namespace algorithm
{
    class ESKF
    {
    public:
        using Scalar = float; // 标量类型

        template <std::size_t ROWS, std::size_t COLS>
        using Matrix = appkit::math::Matrix<Scalar, ROWS, COLS>; // 矩阵类型

        template <std::size_t ROWS>
        using Vector = appkit::math::Vector<Scalar, ROWS>; // 向量类型

        using Quaternion = appkit::math::Quaternion<Scalar>; // 四元数类型

    private:
        static constexpr Matrix<3, 3> I3{appkit::math::identity_matrix<Scalar, 3>()}; // 3x3单位矩阵
        static constexpr Matrix<6, 6> I6{appkit::math::identity_matrix<Scalar, 6>()}; // 6x6单位矩阵

        static constexpr Scalar DEFAULT_CHI_SQUARE_THRESHOLD = 3.841f; // 卡方检验阈值,显著性水平a=0.05时，3自由度的卡方分布临界值约为3.841

        // 名义状态 [quaternion， gyro_bias]
        Quaternion attitude_{}; ///< 姿态四元数
        Vector<3> gyroBias_{};  ///< 陀螺仪偏置

        // 误差状态 [delta_angle, delta_gyro_bias]
        Vector<6> stateErr_{}; // 状态误差
        Vector<3> residual_{}; // 量测残差

        Matrix<6, 6> P{appkit::math::diagonal_matrix<Scalar, 6>(100, 100, 1e-4, 1e-4, 1e-4, 1e-4)}; ///< 状态协方差矩阵
        Matrix<3, 3> R{I3 * 1e-4};                                                                  ///< 量测噪声协方差矩阵

        Matrix<6, 6> F{I6}; ///< 状态转移矩阵
        // Matrix<3, 6> H{};   ///< 量测矩阵
        Matrix<6, 3> K{}; ///< 卡尔曼增益

        Scalar dt_{};                  // 上次更新的时间间隔，单位：秒
        Vector<3> prev_omega_{};       // 上次预测步骤计算的陀螺仪测量值
        Vector<3> prev_delta_angle_{}; // 上次预测步骤计算的增量角

        // 量测状态数据
        Scalar chi_square_value_{0.0f}; ///< 卡方检验值
        Scalar adaptiveGainScale{1.0f}; // 缩放因子
        uint32_t error_count_{0};       ///< 连续异常计数

        // 标志位
        bool is_stable_{false};   ///< imu稳定标志
        bool is_coverage_{false}; ///< 量测覆盖标志

        /**
         * @brief 计算向量的反对称矩阵
         * @param vec 输入向量
         * @return 反对称矩阵
         */
        [[using gnu: optimize("O3")]] static auto skew_symmetric(const Vector<3> &vec)
        {
            // return Eigen::SkewSymmetricMatrix3<Scalar>(vec);
            const auto &[x, y, z] = vec;

            return Matrix<3, 3>{
                0, -z, y,
                z, 0, -x,
                -y, x, 0};
        }

        /**
         * @brief 计算四元数指数映射
         * @param delta_angle 旋转增量，单位：弧度
         * @return 旋转增量对应的四元数
         */
        [[gnu::optimize("O3")]] static Quaternion QuatExp(const Vector<3> &delta_angle)
        {
            // const Scalar angle_square = delta_angle.squaredNorm();
            // if (angle_square > 1e-6f)
            // {
            //     const Scalar angle = std::sqrt(angle_square);
            //     const Scalar half_angle = 0.5f * angle;
            //     const Scalar sin_half = std::sin(half_angle);
            //     const Scalar scale = sin_half / angle;

            //     return Quaternion{
            //         std::cos(half_angle),
            //         scale * delta_angle.x(),
            //         scale * delta_angle.y(),
            //         scale * delta_angle.z()};
            // }
            // else
            // {
            //     // 当旋转增量非常小时，使用泰勒展开近似，避免数值不稳定
            //     return Quaternion{
            //         1.0f - angle_square * 0.125f, // cos(angle/2) ≈ 1 - angle^2/8
            //         0.5f * delta_angle.x(),       // sin(angle/2) ≈ angle/2
            //         0.5f * delta_angle.y(),
            //         0.5f * delta_angle.z()};
            // }

            const float theta = appkit::math::norm(delta_angle);
            const auto &[dx, dy, dz] = delta_angle;

            if (theta < 1e-6f)
            {
                // 当旋转增量非常小时，使用泰勒展开近似，避免数值不稳定
                return Quaternion{
                    1.0f - (theta * theta) * 0.125f, // cos(theta/2) ≈ 1 - theta^2/8
                    0.5f * dx,                       // sin(theta/2) ≈ theta/2
                    0.5f * dy,
                    0.5f * dz};
            }
            else
            {
                const float sin_norm_half = std::sin(0.5f * theta), cos_norm_half = std::cos(0.5f * theta);
                const float scale = sin_norm_half / theta;
                return Quaternion{
                    cos_norm_half,
                    scale * dx,
                    scale * dy,
                    scale * dz};
            }
        }

        /**
         * @brief 修正协方差矩阵，强制其保持对称性
         */
        [[gnu::optimize("O3")]] void CorrectCov()
        {
            for (std::size_t i = 0; i < P.rows(); ++i)
            {
                // 对角线元素最小值约束
                if (const Scalar epsilon = std::numeric_limits<Scalar>::epsilon();
                    P(i, i) <= epsilon) [[unlikely]]
                {
                    P(i, i) = epsilon;
                }

                // 强制对称
                for (std::size_t j = i + 1; j < P.cols(); ++j)
                {
                    auto &upper = P(i, j), &lower = P(j, i);
                    const auto sym_value = 0.5f * (upper + lower);
                    upper = sym_value;
                    lower = sym_value;
                }
            }

            // // 对角线元素最小值约束
            // P.diagonal() = P.diagonal().cwiseMax(std::numeric_limits<Scalar>::epsilon());

            // // 强制对称
            // P = 0.5f * (P + P.transpose());
        }

        /**
         * @brief 预测步骤
         * @param omega 陀螺仪测量值，单位：弧度/秒
         * @param dt 上次更新以来的时间间隔，单位：秒
         */
        void Predict(const Vector<3> &omega, const appkit::Duration &dt);

        /**
         * @brief 更新步骤
         * @param accel_unit 加速度计测量值的单位向量
         */
        void Update(const Vector<3> &accel_unit);

    public:
        // 用户参数
        Scalar chiSquareThreshold = DEFAULT_CHI_SQUARE_THRESHOLD; ///< 卡方检验阈值
        Matrix<3, 3> gyroNoiseCov{I3};                            ///< 陀螺仪噪声协方差
        Matrix<3, 3> gyroBiasNoiseCov{I3 * 1e-4};                 ///< 陀螺仪偏置噪声协方差
        Matrix<3, 3> accelNoiseCov{I3};                           ///< 加速度计噪声协方差

        /**
         * @brief 构造函数
         * @note 默认构造函数假设有较好的初始姿态估计，并且陀螺仪偏置较小。
         */
        constexpr ESKF() noexcept
        {
        }

        /**
         * @brief 初始化姿态
         * @param attitude_init 初始姿态四元数
         */
        [[using gnu: always_inline, optimize("O3")]] void InitPosture(const Quaternion &init)
        {
            attitude_ = init.Normalized();
            // attitude_ = init.normalized(); // Eigen的normalized()函数返回一个新的归一化四元数，保持原四元数不变
        }

        /**
         * @brief 处理新的IMU测量数据，执行预测和更新步骤
         * @param gyro_measurement 陀螺仪测量值，单位：弧度/秒
         * @param accel_measurement 加速度计测量值，单位：米/秒²
         * @param dt 上次更新以来的时间间隔，单位：秒
         * @return 更新后的姿态四元数
         */
        const Quaternion &Process(const Vector<3> &gyro_measurement,
                                  const Vector<3> &accel_measurement,
                                  const appkit::Duration &dt);

        /**
         * @brief 获取当前的姿态估计
         * @return 当前的姿态四元数
         */
        [[using gnu: always_inline, optimize("O3")]] Quaternion GetAttitude() const
        {
            return attitude_;
        }
    };

    [[using gnu: always_inline]] inline void ESKF::Predict(const Vector<3> &omega, const appkit::Duration &dt)
    {
        // 增量角计算 delta_angle = omega * dt + 0.5 * cross(last_omega, omega) * dt^2，考虑陀螺仪测量的变化率对姿态更新的影响
        dt_ = appkit::DurationCast<appkit::second, Scalar>(dt);
        Vector<3> delta_angle_k = 0.5f * (omega + prev_omega_) * dt_; // 使用当前和上次的陀螺仪测量值的平均值计算增量角，提升预测的准确性
        Vector<3> delta_angle = delta_angle_k + (1.0 / 12.0) * appkit::math::cross(prev_delta_angle_, delta_angle_k);
        prev_delta_angle_ = delta_angle_k; // 更新上次的增量角

        // 更新四元数 q = q * exp(delta_angle)，右乘增量四元数，在机体坐标系下更新
        const auto dq = QuatExp(delta_angle);
        (attitude_ *= dq).Normalize();
        // attitude_ = (attitude_ * dq).normalized(); // Eigen的normalized()函数返回一个新的归一化四元数，保持原四元数不变

        // 状态转移矩阵 F = [ [f_angle, f_angle_bias], [0, I] ] = [ [R(delta_angle)^T, -I*dt], [0, I] ]
        appkit::math::make_block<3, 3, 0, 0>(F) = dq.Conjugate().ToRotationMatrix();
        for (std::size_t i = 0; i < 3; ++i)
        {
            F(i, i + 3) = -dt_;
        }
        // F.block<3, 3>(0, 0).noalias() = dq.toRotationMatrix().transpose(); // 旋转矩阵的转置等于其逆矩阵，直接计算转置避免不必要的矩阵乘法，提升性能
        // F.block<3, 3>(0, 3).diagonal().setConstant(-dt_);                  // 角度-偏置部分的状态转移矩阵为-I*dt

        // 协方差预测 P = F * P * F^T + Q
        {
            P = appkit::math::eval(F * P) * appkit::math::transpose(F);

            // 直接加上过程噪声协方差
            Scalar dt_s_square = dt_ * dt_;
            appkit::math::make_block<3, 3, 0, 0>(P) += gyroNoiseCov;
            appkit::math::make_block<3, 3, 3, 3>(P) += gyroBiasNoiseCov * dt_s_square; // 角度-偏置部分的过程噪声

            // P = (F * P).eval() * F.transpose();

            // // 直接加上过程噪声协方差
            // Scalar dt_s_square = dt_ * dt_;
            // P.block<3, 3>(0, 0).noalias() += gyroNoiseCov;
            // P.block<3, 3>(3, 3).noalias() += gyroBiasNoiseCov * dt_s_square; // 角度-偏置部分的过程噪声
        }

        // 修正协方差矩阵，保持数值稳定
        CorrectCov();
    }

    [[using gnu: always_inline]] inline void ESKF::Update(const Vector<3> &accel_unit)
    {
        // 计算重力加速度在机体坐标系下的表示
        const auto &[w, x, y, z] = attitude_;
        // const auto w = attitude_.w(), x = attitude_.x(), y = attitude_.y(), z = attitude_.z();
        Vector<3> g_body = Vector<3>{
            2.0f * (w * y - x * z),
            -2.0f * (w * x + y * z),
            2.0f * (x * x + y * y) - 1.0f};

        // 计算残差 r = z - h(x) = accel_unit - g_body
        residual_ = accel_unit - g_body;

        // 量测矩阵 H[delta_angle, delta_gyro_bias] = [[g_body]_x, 0]
        const Matrix<3, 3> skew_g_body = skew_symmetric(g_body);
        // appkit::math::make_block<3, 3, 0, 0>(H) = skew_g_body;

        // 计算卡尔曼增益 K = P * H^T * (H * P * H^T + R)^-1
        // Matrix<3, 3> S = H * P * appkit::math::transpose(H) + R, invS;
        Matrix<3, 3> S = appkit::math::make_block<3, 3, 0, 0>(skew_g_body * appkit::math::make_block<3, 6, 0, 0>(P)) * appkit::math::transpose(skew_g_body) + R,
                     invS;
        if (appkit::math::inv_to(S, invS) == false) [[unlikely]]
        {
            // 矩阵不可逆，进入失效状态
            is_coverage_ = false;
            return;
        }
        // Matrix<3, 3> S = (skew_g_body * P.block<3, 6>(0, 0)).block<3, 3>(0, 0).eval() * skew_g_body.transpose() + R,
        //              invS = S.inverse(); // 直接计算逆矩阵，避免不必要的矩阵乘法，提升性能
        // if (Eigen::FullPivLU<Matrix<3, 3>> lu(S); lu.isInvertible()) [[unlikely]]
        // {
        //     // 矩阵不可逆，进入失效状态
        //     is_coverage_ = false;
        //     return;
        // }
        // else
        // {
        //     invS.noalias() = lu.inverse();
        // }

        // 计算马氏距离 chi_square_value = r^T * S^-1 * r
        chi_square_value_ = (appkit::math::transpose(residual_) * invS * residual_)(0, 0);
        if (chi_square_value_ <= 0)
        {
            is_coverage_ = false; // 量测异常，进入失效状态
            return;
        }

        // 根据卡方检验结果更新覆盖状态和异常计数器
        if (chi_square_value_ < 0.5 * chiSquareThreshold)
        {
            is_coverage_ = true;
        }

        if (chi_square_value_ > chiSquareThreshold && is_coverage_)
        {
            if (is_stable_)
            {
                error_count_++;
            }
            else
            {
                error_count_ = 0; // 如果IMU不稳定，重置计数器
            }

            if (error_count_ > 50) // 连续异常超过50次，认为量测异常
            {
                // 滤波器发散
                is_coverage_ = false; // 认为量测不可靠，进入失效状态

                // 膨胀协方差
                P *= 1.5; // 适当增加协方差，提升滤波器的鲁棒性，防止过度自信导致发散
            }
            else
            {
                // 残差未通过卡方检验 仅预测
                return;
            }
        }
        else
        {
            // 应用自适应增益缩放
            if (chi_square_value_ > 0.1 * chiSquareThreshold && is_coverage_) // 量测接近异常阈值时，适当降低增益
            {
                adaptiveGainScale = (chiSquareThreshold - chi_square_value_) / (0.9f * chiSquareThreshold);
            }
            else
            {
                adaptiveGainScale = 1.0f; // 使用全增益
            }

            error_count_ = 0; // 重置异常计数器
        }

        // 计算卡尔曼增益 K = P * H^T * (H * P * H^T + R)^-1
        K = appkit::math::make_block<6, 3, 0, 0>(P) * appkit::math::transpose(skew_g_body) * invS;
        // K.noalias() = (P.block<6, 3>(0, 0) * skew_g_body.transpose()).eval() * invS;

        // 更新状态估计 delta_x = K * r
        stateErr_ = (K * residual_) * adaptiveGainScale; // 根据残差和增益计算状态误差，并应用缩放

        // 注入误差
        {
            // 更新姿态估计 q = exp(delta_angle) * q = q * exp(q^-1 * delta_angle)，左乘增量四元数，在全局坐标系下更新
            Vector<3> delta_angle = appkit::math::make_block<3, 1, 0, 0>(stateErr_); // 将误差从机体坐标系转换到全局坐标系
            delta_angle -= g_body * appkit::math::dot(g_body, delta_angle);          // 屏蔽绕重力方向的更新
            // Vector<3> delta_angle = stateErr_.head<3>();                 // 将误差从机体坐标系转换到全局坐标系
            // delta_angle.noalias() -= g_body * (g_body.dot(delta_angle)); // 屏蔽绕重力方向的更新

            // 姿态误差限幅
            if (const auto angle_norm = appkit::math::norm(delta_angle);
                angle_norm > 0.5f) [[unlikely]]
            {
                delta_angle *= (0.5f / angle_norm); // 将误差限制在0.5弧度以内，防止姿态更新过大导致发散
            }

            (attitude_ *= QuatExp(delta_angle)).Normalize(); // 右乘增量四元数，在机体坐标系下更新,归一化
            // attitude_ = (attitude_ * QuatExp(delta_angle)).normalized(); // 右乘增量四元数，在机体坐标系下更新,归一化

            // 更新陀螺仪偏置 b = b + delta_gyro_bias
            if (is_coverage_)
            {
                Vector<3> delta_bias = appkit::math::make_block<3, 1, 3, 0>(stateErr_); // 从状态误差中提取陀螺仪偏置更新量
                delta_bias -= g_body * appkit::math::dot(g_body, delta_bias);           // 屏蔽绕重力方向的偏置更新
                // Vector<3> delta_bias = stateErr_.tail<3>();                // 从状态误差中提取陀螺仪偏置更新量
                // delta_bias.noalias() -= g_body * (g_body.dot(delta_bias)); // 屏蔽绕重力方向的偏置更新

                const Scalar bias_limit = 1e-3 * dt_; // 偏置更新的最大绝对值，单位：弧度/秒
                // gyroBias_.noalias() += delta_bias.cwiseMax(-bias_limit).cwiseMin(bias_limit); // 限制偏置更新的幅度，防止发散
                gyroBias_ = appkit::math::cwise_clamp(delta_bias, -bias_limit, bias_limit);
            }
        }

        // 更新协方差矩阵 P = (I - K * H) * P * (I - K * H)^T + K * R * K^T
        {
            Matrix<6, 6> I_KH = I6;
            appkit::math::make_block<6, 3, 0, 0>(I_KH) -= K * skew_g_body; // I - K * H
            P = appkit::math::eval(I_KH * P) * appkit::math::transpose(I_KH) +
                appkit::math::eval(K * R) * appkit::math::transpose(K); // Joseph形式更新协方差矩阵，提升数值稳定性
            // P = I_KH * P * appkit::math::transpose(I_KH) + K * R * appkit::math::transpose(K); // Joseph形式更新协方差矩阵，提升数值稳定性

            // Matrix<6, 6> I_KH = I6;
            // I_KH.block<6, 3>(0, 0).noalias() -= K * skew_g_body;     // I - K * H
            // P = I_KH * P * I_KH.transpose() + K * R * K.transpose(); // Joseph形式更新协方差矩阵，提升数值稳定性
        }

        // 协方差的微调
        // 一般因为误差角度很小, 旋转矩阵近乎为单位矩阵，省略

        // 修正协方差矩阵，保持数值稳定
        CorrectCov();
    }

    [[gnu::optimize("O3")]] inline const ESKF::Quaternion &ESKF::Process(const Vector<3> &gyro,
                                                                         const Vector<3> &accel,
                                                                         const appkit::Duration &dt)
    {
        // 计算陀螺仪测量的预测真实角速度
        const Vector<3> omega = gyro - gyroBias_;

        // 执行预测步骤
        Predict(omega, dt);

        // 判断IMU是否稳定：加速度计测量接近重力加速度且陀螺仪测量较小
        const Scalar accel_norm = appkit::math::norm(accel),
                     abs_modified_accel_norm = std::abs(accel_norm - WORLD_GRAVITY),
                     omega_norm = appkit::math::norm(omega);
        is_stable_ = (abs_modified_accel_norm < 0.3f && omega_norm < 0.3f);
        // const Scalar accel_norm = accel.norm(),
        //              abs_modified_accel_norm = std::abs(accel_norm - WORLD_GRAVITY),
        //              omega_norm = omega.norm();
        // is_stable_ = (abs_modified_accel_norm < 0.3f && omega_norm < 0.3f);

        // 量测噪声矩阵
        const Scalar measurement_noise_scale =
            (1.0f + 0.5f * abs_modified_accel_norm)             // 根据加速度计测量的稳定程度动态调整量测噪声协方差，测量越接近重力加速度，噪声越小
            * (1.0f + 0.5f * omega_norm)                        // 根据陀螺仪测量的稳定程度动态调整量测噪声协方差，测量越小，噪声越小
            * (1.0f + (chi_square_value_ / chiSquareThreshold)) // 根据卡方检验结果动态调整量测噪声协方差，残差越大，噪声越大
            * (is_coverage_ ? 1.0f : 10.0f);                    // 如果量测覆盖异常，增加量测噪声协方差，降低滤波器对异常量测的敏感度
        R = accelNoiseCov * (measurement_noise_scale / (WORLD_GRAVITY * WORLD_GRAVITY));

        // 使用归一化的加速度测量进行更新
        if (accel_norm > 1e-3f) // 避免除以零
            Update(accel / accel_norm);

        prev_omega_ = omega; // 更新上次的陀螺仪测量值
        return attitude_;
    }
} // namespace algorithm
