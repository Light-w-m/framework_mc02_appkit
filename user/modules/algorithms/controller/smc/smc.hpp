/**
 * @file smc.hpp
 * @brief SMC滑膜控制器
 * @author Light 
 */
#pragma once

#include <common_time.hpp>

namespace algorithm
{
    /**
     * @brief SMC 滑膜控制器
     */
    class SMC
    {      
    public:
        /**
         * @brief SMC参数结构体
         */
        struct Param final
        {
            float C{}; ///< 滑模面系数
            float K{}; ///< 切换增益
            float error_eps{};///<误差下限
            float u_max{};///<输出最大值
            float epsilon{};///<趋近率指数增益
            float phi{1.0f};///<滑模面厚度

            float gx{};///<控制器增益
        };

        /**
         * @brief 构造函数
         * @param param SMC参数结构体
         */
        constexpr SMC(Param param) noexcept : param_(param) {}

        /**
         * @brief 更新SMC控制器
         * @param x 状态量
         * @param dx 状态量一阶导
         * @param target 期望目标值
         * @param dt 时间间隔
         * @return 控制输出
         */
        float Update(float x, float dx, float target, const appkit::Duration &dt) noexcept;

        /**
         * @brief 重置SMC状态
         */
        void Reset() noexcept;

    private:
        Param param_;///<参数结构体

        float error_{};///<误差
        float error_last_{};///<上一次的误差
        float dref_{};///<目标值一阶导
        float ddref_{};///<目标值二阶导
        float ref_last_{};///<上一次的目标值

        float s_;///<滑模面
        float u_;///<控制输出
        float x_;///<状态量
        float dx_;
          
        /**
         * @brief 饱和函数
         * 
         * @param x 
         * @return float 
         */
        float Sat(float x) const noexcept;

        /**
         * @brief 符号函数,若有抖动可以换个陡峭的饱和函数
         * 
         * @param x 
         * @return float 
         */
        float Sign(float x) const noexcept;

    };
} // namespace algorithm
