#pragma once

#include <matrix.hpp>

namespace algorithm
{
    /**
     * @brief 递归最小二乘（RLS）算法类
     * @tparam N 输入向量维度
     * @tparam T 数值类型，默认为float
     */
    template <std::size_t N, typename T = float>
        requires std::conjunction_v<std::is_arithmetic<T>, std::bool_constant<(N > 0)>>
    class RLS final
    {
    private:
        appkit::math::Vector<T, N> weights_; ///< 权重向量
        appkit::math::Matrix<T, N, N> P_;    ///< 协方差矩阵

        const T lambda_; ///< 遗忘因子
        const T delta_;  ///< 初始协方差缩放因子

    public:
        /**
         * @brief 构造函数
         * @param lambda 遗忘因子，取值范围(0, 1]
         * @param delta 初始协方差缩放因子，通常取较大的正值
         */
        constexpr explicit RLS(std::convertible_to<T> auto lambda, std::convertible_to<T> auto delta) noexcept
            : lambda_(static_cast<T>(lambda)), delta_(static_cast<T>(delta))
        {
            Reset();
        }

        /**
         * @brief 重置RLS算法状态
         */
        constexpr void Reset() noexcept
        {
            weights_ = appkit::math::Vector<T, N>{}; // 初始化权重向量为零
            P_ = appkit::math::identity_matrix<T, N>() * delta_;
        }

        /**
         * @brief 获取当前权重向量
         * @return 权重向量常量引用
         */
        FORCE_INLINE constexpr const appkit::math::Vector<T, N> &GetWeights() const noexcept
        {
            return weights_;
        }

        /**
         * @brief 设置权重向量
         * @param new_weights 新的权重向量
         */
        FORCE_INLINE constexpr void SetWeights(const appkit::math::Vector<T, N> &new_weights) noexcept
        {
            weights_ = new_weights;
        }

        /**
         * @brief 获取当前协方差矩阵
         * @return 协方差矩阵引用
         */
        FORCE_INLINE constexpr const appkit::math::Matrix<T, N, N> &GetCovarianceMatrix() const noexcept
        {
            return P_;
        }

        /**
         * @brief 使用新的输入和期望输出更新RLS算法状态
         * @param input 输入向量
         * @param desired_output 期望输出值
         * @return 更新后的权重向量常量引用
         */
        constexpr const appkit::math::Vector<T, N> &Update(const appkit::math::Vector<T, N> &input, std::convertible_to<T> auto desired_output) noexcept
        {
            using namespace appkit::math;

            const T d = static_cast<T>(desired_output);

            // 计算增益向量
            const Vector<T, N> k = (P_ * input) / (lambda_ + (transpose(input) * P_ * input)(0, 0));

            // 计算误差
            const T error = d - (transpose(input) * weights_)(0, 0);

            // 更新权重向量
            weights_ += k * error;

            // 更新逆相关矩阵
            P_ = (P_ - k * transpose(input) * P_) / lambda_;

            return weights_;
        }
    };
} // namespace algorithm
