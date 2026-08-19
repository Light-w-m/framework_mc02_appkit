#pragma once

#include <macros.hpp>
#include <algorithm>

namespace algorithm
{
    /**
     * @brief MIT控制器
     * @note output = kp * (pos - pos_measure) + kd * (spd - spd_measure) + torque_ff
     */
    class MIT final
    {
    public:
        /**
         * @brief MIT参数结构体
         */
        struct Param final
        {
            float kp{}; ///< 比例系数
            float ki{}; ///< 积分系数
            float kd{}; ///< 微分系数

            float max_output{}; ///< 最大输出，等于0表示不限制
        };

        /**
         * @brief 构造函数
         * @param param MIT参数结构体
         */
        constexpr MIT(Param param) noexcept : param_(param) {}

        /**
         * @brief 更新MIT控制器
         * @param pos_target 位置目标值
         * @param spd_target 速度目标值
         * @param pos_measure 位置测量值
         * @param spd_measure 速度测量值
         * @param torque_ff 力矩前馈值
         * @return 控制输出
         */
        float Update(float pos_target, float spd_target, float pos_measure, float spd_measure, float torque_ff) noexcept;

        /**
         * @brief 重置MIT状态
         */
        void Reset() noexcept;

        /**
         * @brief 获取MIT参数
         * @return MIT参数结构体
         */
        FORCE_INLINE const Param &GetParam() const noexcept
        {
            return param_;
        }

        /**
         * @brief 设置MIT参数
         * @param param MIT参数结构体
         */
        FORCE_INLINE void SetParam(const Param &param) noexcept
        {
            param_ = param;
        }

    private:
        Param param_; // MIT参数

        float pOut_{}; ///< 比例项输出
        float dOut_{}; ///< 微分项输出

        /**
         * @brief 最大输出限幅
         * @param output 输出值
         */
        void f_MaxOutput(float &output) const noexcept;
    };
} // namespace algorithm
