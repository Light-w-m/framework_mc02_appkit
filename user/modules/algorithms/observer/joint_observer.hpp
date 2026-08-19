#pragma once

#include <common_time.hpp>
#include <rls.hpp>

namespace algorithm
{
    /**
     * @brief 关节观测器类
     * @note 基于递归最小二乘法（RLS）实现关节参数辨识
     */
    class JointObserver final
    {
    public:
        /**
         * @brief 构造函数
         * @param rls_lambda RLS遗忘因子
         * @param rls_delta RLS初始协方差缩放因子
         * @param tau_epsilon 力矩常数微小值
         */
        explicit JointObserver(float rls_lambda = 0.99, float rls_delta = 1e3, float tau_epsilon = 1e-5) noexcept;

        /**
         * @brief 设置权重向量
         * @param new_weights 新的权重向量
         * @note 权重向量顺序为 [转动惯量, 阻尼系数, 力矩常数, 重力项]
         */
        FORCE_INLINE void SetWeights(const appkit::math::Vector<float, 4> &new_weights) noexcept
        {
            rls_.SetWeights(new_weights);
        }

        /**
         * @brief 析构函数
         */
        ~JointObserver() = default;

        /**
         * @brief 使用新的输入和输出更新关节观测器状态
         * @param angle 关节角度，单位：弧度
         * @param omega 关节角速度，单位：弧度每秒
         * @param torque 关节输出力矩，单位：牛·米
         * @param dt 采样时间间隔
         * @return 更新后的关节参数向量常量引用 [转动惯量, 阻尼系数, 力矩常数, 重力项]
         */
        constexpr const appkit::math::Vector<float, 4> &Update(float angle, float omega, float torque, appkit::Duration dt) noexcept;

    private:
        RLS<4, float> rls_;       ///< RLS对象
        const float tau_epsilon_; ///< 力矩常数微小值

        float last_omega_{}; ///< 上次关节角速度
        float beta_{};       ///< 角加速度

        /**
         * @brief 饱和值函数
         * @param value 输入值
         * @return 饱和值
         */
        constexpr float SignSat(float value) const noexcept;
    };
}