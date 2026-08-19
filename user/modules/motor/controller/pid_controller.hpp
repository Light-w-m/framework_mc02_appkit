#pragma once

#include <motor.hpp>
#include <pid.hpp>

namespace control::motor
{
    /**
     * @brief 多级位置式PID控制器模板类
     * @tparam MotorType 电机类型，必须继承自Motor类
     * @tparam N PID数量
     */
    template <typename MotorType, size_t N>
        requires(N >= 1)
    class PIDController final : public Controller<MotorType>
    {
    public:
        static constexpr float ZeroData = 0.0f; ///< 零数据常量

        /**
         * @brief PID实现结构体
         */
        struct PIDImpl final
        {
            algorithm::PositionPID pid;                      ///< 位置式PID实例
            std::reference_wrapper<const float> measure;     ///< 测量值引用
            std::reference_wrapper<const float> feedforward; ///< 前馈值引用

            bool enabled{true};          ///< PID使能标志
            bool target_reverse{false};  ///< 目标值反向标志
            bool measure_reverse{false}; ///< 测量值反向标志

            /**
             * @brief 构造函数
             * @param param 位置式PID参数
             */
            PIDImpl(const algorithm::PID::Param &param) noexcept
                : pid(param), measure(ZeroData), feedforward(ZeroData)
            {
            }
        };

        /**
         * @brief 构造函数
         * @param motor 电机实例引用
         * @param pid_params PID参数数组引用
         */
        constexpr PIDController(MotorType &motor, std::initializer_list<const algorithm::PID::Param> pid_params) noexcept
            : motor_(motor), pidImpls_(pid_params)
        {
        }

        /**
         * @brief 获取指定索引的PID实例const
         * @tparam Index 索引
         * @return 指定索引的PID实例引用
         */
        template <size_t Index = 0>
            requires(Index < N)
        FORCE_INLINE PIDImpl &GetPIDImpl() noexcept
        {
            return pidImpls_[Index];
        }

        /**
         * @brief 设置指定索引的测量值引用
         * @tparam Index 索引
         * @param measure 测量值引用
         */
        template <size_t Index = 0>
            requires(Index < N)
        FORCE_INLINE void SetMeasure(const float &measure) noexcept
        {
            pidImpls_[Index].measure = std::cref(measure);
        }

        /**
         * @brief 设置指定索引的前馈值引用
         * @tparam Index 索引
         * @param feedforward 前馈值引用
         */
        template <size_t Index = 0>
            requires(Index < N)
        FORCE_INLINE void SetFeedforward(const float &feedforward) noexcept
        {
            pidImpls_[Index].feedforward = std::cref(feedforward);
        }

        /**
         * @brief 更新PID控制器
         * @param target 目标值
         * @param dt 时间间隔
         * @return 错误码
         */
        appkit::ErrorCode Update(const float target, appkit::Duration dt)
        {
            float output = target;

            // 依次更新每个PID
            for (size_t i = 0; i < N; ++i)
            {
                PIDImpl &pid_impl = pidImpls_[i];

                // 检查PID是否使能
                if (pid_impl.enabled)
                {
                    const float pid_target = pid_impl.target_reverse ? -output : output;
                    const float pid_measure = pid_impl.measure_reverse ? -pid_impl.measure.get() : pid_impl.measure.get();

                    output = pid_impl.pid.Update(
                        pid_target,
                        pid_measure,
                        dt);

                    output += pid_impl.feedforward.get();
                }
            }

            // 最终输出给电机控制
            return motor_.TorqueControl(output);
        }

    private:
        MotorType &motor_;    ///< 电机实例引用
        PIDImpl pidImpls_[N]; ///< PID实现数组
    };
}