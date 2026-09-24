#pragma once

#include <common_type.hpp>
#include <cmath>
#include <numbers>

namespace algorithm
{
    /**
     * @brief Butterworth滤波器类型枚举
     */
    enum class ButterworthFilterType
    {
        LOWPASS,  ///< 低通滤波器
        HIGHPASS, ///< 高通滤波器
        BANDPASS, ///< 带通滤波器
        BANDSTOP  ///< 带阻滤波器
    };

    /**
     * @brief Butterworth滤波器模板类
     * @tparam FilterType 滤波器类型
     */
    template <ButterworthFilterType FilterType>
    class ButterworthFilter
    {
    private:
        // 滤波器系数
        float b0, b1, b2; // 分子系数
        float a1, a2;     // 分母系数 (a0归一化为1)

        // 状态变量
        float x1, x2;

        // 滤波器参数
        float fs; // 采样率

    public:
        /**
         * @brief 构造函数
         * @param sampling_rate 采样率（Hz）
         * @param cutoff_freq 截止频率（Hz）
         * @note 仅支持低通和高通滤波器的设计
         */
        constexpr ButterworthFilter(float sampling_rate, float cutoff_freq) noexcept
            : x1(0.0f), x2(0.0f), fs(sampling_rate)
        {
            static_assert(appkit::RangeIn(FilterType, {ButterworthFilterType::LOWPASS, ButterworthFilterType::HIGHPASS}),
                          "Current constructor only supports LOWPASS and HIGHPASS types.");

            const float wc = 2.0f * std::numbers::pi_v<float> * cutoff_freq; // 模拟角频率
            const float T = 1.0f / fs;                                       // 采样周期

            const float wa = (2.0f / T) * std::tan(wc * T / 2.0f); // 数字预畸变角频率
            const float wa2 = wa * wa;

            if constexpr (FilterType == ButterworthFilterType::LOWPASS)
            {
                // 模拟低通传递函数：H(s) = wc^2 / (s^2 + sqrt(2)*wc*s + wc^2)
                // 双线性变换到数字域
                const float den = wa2 + std::numbers::sqrt2_v<float> * wa + 1;

                b0 = wa2 / den;
                b1 = 2.0f * b0;
                b2 = b0;
                a1 = (2.0f * (1 - wa2)) / den;
                a2 = (wa2 - std::numbers::sqrt2_v<float> * wa + 1) / den;
            }
            else if constexpr (FilterType == ButterworthFilterType::HIGHPASS)
            {
                // 模拟高通传递函数：H(s) = s^2 / (s^2 + sqrt(2)*wc*s + wc^2)
                // 双线性变换到数字域
                const float den = wa2 + std::numbers::sqrt2_v<float> * wa + 1;

                b0 = 1 / den;
                b1 = -2.0f * b0;
                b2 = b0;
                a1 = (2.0f * (1 - wa2)) / den;
                a2 = (wa2 - std::numbers::sqrt2_v<float> * wa + 1) / den;
            }
        }

        /**
         * @brief 构造函数
         * @param sampling_rate 采样率（Hz）
         * @param lowerFreq 带通/带阻滤波器的下截止频率（Hz）
         * @param upperFreq 带通/带阻滤波器的上截止频率（Hz）
         * @note 仅支持带通和带阻滤波器的设计
         */
        constexpr ButterworthFilter(float sampling_rate, float lowerFreq, float upperFreq) noexcept
            : x1(0.0f), x2(0.0f), fs(sampling_rate)
        {
            static_assert(appkit::RangeIn(FilterType, {ButterworthFilterType::BANDPASS, ButterworthFilterType::BANDSTOP}),
                          "Current constructor only supports BANDPASS and BANDSTOP types.");

            // 确保频率顺序正确
            if (lowerFreq >= upperFreq)
            {
                std::swap(lowerFreq, upperFreq);
            }

            const float T = 1.0f / fs; // 采样周期

            // 计算带宽和中心频率
            float w1 = 2.0 * std::numbers::pi_v<float> * lowerFreq;
            float w2 = 2.0 * std::numbers::pi_v<float> * upperFreq;

            // 预扭曲
            float w1_p = 2.0 / T * std::tan(w1 * T / 2.0);
            float w2_p = 2.0 / T * std::tan(w2 * T / 2.0);
            float w0_p = std::sqrt(w1_p * w2_p);
            float B_p = w2_p - w1_p;

            if constexpr (FilterType == ButterworthFilterType::BANDPASS)
            {
                // 带通滤波器设计
                float den = w0_p * w0_p + B_p * w0_p + B_p * B_p / 4.0;

                b0 = (B_p * w0_p) / den;
                b1 = 0.0;
                b2 = -(B_p * w0_p) / den;
                a1 = (2.0 * w0_p * w0_p - 2.0 * B_p * B_p / 4.0) / den;
                a2 = (w0_p * w0_p - B_p * w0_p + B_p * B_p / 4.0) / den;
            }
            else if constexpr (FilterType == ButterworthFilterType::BANDSTOP)
            {
                // 带阻滤波器设计
                float den = w0_p * w0_p + B_p * w0_p + B_p * B_p / 4.0;

                b0 = (w0_p * w0_p + B_p * B_p / 4.0) / den;
                b1 = (2.0 * w0_p * w0_p - 2.0 * B_p * B_p / 4.0) / den;
                b2 = (w0_p * w0_p + B_p * B_p / 4.0) / den;
                a1 = (2.0 * w0_p * w0_p - 2.0 * B_p * B_p / 4.0) / den;
                a2 = (w0_p * w0_p - B_p * w0_p + B_p * B_p / 4.0) / den;
            }
        }

        /**
         * @brief 重置滤波器状态
         */
        constexpr void Reset() noexcept
        {
            x1 = 0.0f;
            x2 = 0.0f;
        }

        /**
         * @brief 处理输入信号并返回滤波后的输出
         * @param input 输入信号
         * @return 滤波后的输出信号
         */
        constexpr float Process(float input) noexcept
        {
            // 计算当前输出
            float output = b0 * input + x1;

            // 更新状态变量
            x1 = b1 * input - a1 * output + x2;
            x2 = b2 * input - a2 * output;

            return output;
        }
    };
} // namespace algorithm
