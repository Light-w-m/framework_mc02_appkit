#pragma once

#include <motor.hpp>
#include <mit.hpp>

namespace control::motor
{
    /**
     * @brief MIT控制器模板类
     * @tparam MotorType 电机类型，必须继承自Motor类
     */
    template <typename MotorType>
    class MITController final : public Controller<MotorType>
    {
    public:
        /**
         * @brief 构造函数
         * @param motor 电机实例引用
         * @param mit_param MIT参数结构体
         */
        constexpr MITController(MotorType &motor, const algorithm::MIT::Param &mit_param) noexcept
            : motor_(motor), mit_(mit_param)
        {
            // 默认测量值引用指向电机测量数据
            position_measure_ = std::cref(motor_.GetMeasure().single_pos);
            speed_measure_ = std::cref(motor_.GetMeasure().speed);
        }

        /**
         * @brief 设置位置测量值引用
         * @param position_measure 位置测量值引用
         */
        FORCE_INLINE void SetPositionMeasure(const float &position_measure) noexcept
        {
            position_measure_ = std::cref(position_measure);
        }

        /**
         * @brief 设置速度测量值引用
         * @param speed_measure 速度测量值引用
         */
        FORCE_INLINE void SetSpeedMeasure(const float &speed_measure) noexcept
        {
            speed_measure_ = std::cref(speed_measure);
        }

        /**
         * @brief 更新MIT控制器
         * @param position_target 位置目标值
         * @param speed_target 速度目标值
         * @param torque_feedforward 力矩前馈值
         * @return 错误码
         */
        appkit::ErrorCode Update(float position_target, float speed_target, float torque_feedforward)
        {
            float output = mit_.Update(
                position_target,
                speed_target,
                position_measure_.get(),
                speed_measure_.get(),
                torque_feedforward);

            return motor_.TorqueControl(output);
        }

    private:
        MotorType &motor_;   ///< 电机实例引用
        algorithm::MIT mit_; ///< MIT控制器实例

        std::reference_wrapper<const float> position_measure_; ///< 位置测量值引用
        std::reference_wrapper<const float> speed_measure_;    ///< 速度测量值引用
    };
}