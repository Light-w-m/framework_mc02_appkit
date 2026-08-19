#pragma once

#if __has_include(<main.h>)
#include <main.h>
#endif

#ifdef HAL_TIM_MODULE_ENABLED

#if __has_include(<tim.h>)
#include <tim.h>
#else
#error "Please include tim.h in your project"
#endif

#include <timebase.hpp>
#include <pwm.hpp>

namespace appkit::stm32
{
    /**
     * @brief 定时器ID枚举定义
     */
    enum class TimerId
    {
#ifdef TIM1
        STM32_TIM1,
#endif
#ifdef TIM2
        STM32_TIM2,
#endif
#ifdef TIM3
        STM32_TIM3,
#endif
#ifdef TIM4
        STM32_TIM4,
#endif
#ifdef TIM5
        STM32_TIM5,
#endif
#ifdef TIM6
        STM32_TIM6,
#endif
#ifdef TIM7
        STM32_TIM7,
#endif
#ifdef TIM8
        STM32_TIM8,
#endif
#ifdef TIM9
        STM32_TIM9,
#endif
#ifdef TIM10
        STM32_TIM10,
#endif
#ifdef TIM11
        STM32_TIM11,
#endif
#ifdef TIM12
        STM32_TIM12,
#endif
#ifdef TIM13
        STM32_TIM13,
#endif
#ifdef TIM14
        STM32_TIM14,
#endif
        STM32_TIMER_NUMBER,
        STM32_TIMER_ID_ERROR
    };

    /**
     * @brief 获取定时器ID
     * @param handle 定时器句柄
     * @return 定时器ID
     */
    constexpr TimerId GetTimerId(const TIM_HandleTypeDef *handle);

    /**
     * @brief STM32定时器封装
     */
    class STM32Timer final : public TimeBase::Block
    {
        static inline STM32Timer *map_[std::to_underlying(TimerId::STM32_TIMER_NUMBER)]{}; ///< 定时器对象映射表

        /* Handle */
        TIM_HandleTypeDef *handle_{}; ///< 定时器句柄

        /* Config */
        const bool as_clock_{false}; ///< 是否作为时钟源使用
        const bool is_32bit_{false}; ///< 是否为32位定时器

        /* Clock */
        std::atomic_size_t cycle_count{}; ///< 周期计数
        Duration period_{};     ///< 定时器周期

        /* Base */
        uint32_t tim_source_freq_{}; ///< 定时器时钟源频率
        float min_frequency_{0.0f};  ///< 最小支持频率

        /**
         * @brief 获取定时器句柄
         * @return 定时器句柄
         */
        FORCE_INLINE TIM_HandleTypeDef *GetTIMHandle()
        {
            return handle_;
        }

        /**
         * @brief 获取最小支持频率
         * @return 最小支持频率，单位Hz
         */
        FORCE_INLINE float GetMinFrequency() const
        {
            return min_frequency_;
        }

        /**
         * @brief 获取当前频率
         * @return 频率，单位Hz
         */
        FORCE_INLINE float GetFrequency() const
        {
            return tim_source_freq_ / static_cast<float>(__HAL_TIM_GET_AUTORELOAD(handle_));
        }

        /**
         * @brief 获取指定通道的占空比
         * @param channel 通道号
         * @return 占空比，范围0.0~1.0
         */
        float GetDutyCycle(uint32_t channel) const;

    public:
        /**
         * @brief PWM通道封装
         */
        class Channel final : public PWM::Block
        {
            STM32Timer &parent_; ///< 所属定时器对象
            uint32_t channel_{}; ///< 通道号

        public:
            /**
             * @brief 构造函数
             * @param name 通道名称
             * @param parent 所属定时器对象
             * @param channel 通道号
             */
            Channel(const char *name, STM32Timer &parent, uint32_t channel);

            /**
             * @brief 应用修改
             * @param modify 修改类型
             * @return 错误码
             */
            virtual ErrorCode ApplyModify(PWM::ModifyState modify) override;
        };

        /**
         * @brief 构造函数
         * @param name 定时器名称
         * @param handle 定时器句柄
         * @param as_clock 是否作为时钟源使用
         */
        STM32Timer(const char *name, TIM_HandleTypeDef &handle, bool as_clock = false);

        ~STM32Timer();

        /**
         * @brief 获取当前时间点
         * @return 当前时间点
         */
        virtual TimePoint Now() const override;

        /**
         * @brief 设置频率
         * @param frequency 频率，单位Hz
         * @return 错误码
         */
        ErrorCode SetFrequency(uint32_t frequency);

        /**
         * @brief 设置占空比
         * @param channel 通道号
         * @param duty_cycle 占空比，范围0.0~1.0
         * @return 错误码
         */
        ErrorCode SetDutyCycle(uint32_t channel, float duty_cycle);

        /**
         * @brief 定时器中断回调函数
         * @param htim 定时器句柄
         */
        static void TimerCallback(TIM_HandleTypeDef *htim);
    };
}

#endif