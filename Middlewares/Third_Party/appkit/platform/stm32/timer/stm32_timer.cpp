#include <stm32_timer.hpp>

#ifdef HAL_TIM_MODULE_ENABLED

#include <logger.hpp>

#if USE_HAL_TIM_REGISTER_CALLBACKS == 0
#pragma message "Using default HAL_TIM_PeriodElapsedCallback, make sure no other module (like hal_tick) uses it."
#endif

namespace appkit::stm32
{
    constexpr TimerId GetTimerId(const TIM_HandleTypeDef *handle)
    {
        const auto addr = handle->Instance;
        if (addr == nullptr)
            return TimerId::STM32_TIMER_ID_ERROR;
#ifdef TIM1
        if (addr == TIM1)
            return TimerId::STM32_TIM1;
#endif
#ifdef TIM2
        if (addr == TIM2)
            return TimerId::STM32_TIM2;
#endif
#ifdef TIM3
        if (addr == TIM3)
            return TimerId::STM32_TIM3;
#endif
#ifdef TIM4
        if (addr == TIM4)
            return TimerId::STM32_TIM4;
#endif
#ifdef TIM5
        if (addr == TIM5)
            return TimerId::STM32_TIM5;
#endif
#ifdef TIM6
        if (addr == TIM6)
            return TimerId::STM32_TIM6;
#endif
#ifdef TIM7
        if (addr == TIM7)
            return TimerId::STM32_TIM7;
#endif
#ifdef TIM8
        if (addr == TIM8)
            return TimerId::STM32_TIM8;
#endif
#ifdef TIM9
        if (addr == TIM9)
            return TimerId::STM32_TIM9;
#endif
#ifdef TIM10
        if (addr == TIM10)
            return TimerId::STM32_TIM10;
#endif
#ifdef TIM11
        if (addr == TIM11)
            return TimerId::STM32_TIM11;
#endif
#ifdef TIM12
        if (addr == TIM12)
            return TimerId::STM32_TIM12;
#endif
#ifdef TIM13
        if (addr == TIM13)
            return TimerId::STM32_TIM13;
#endif
#ifdef TIM14
        if (addr == TIM14)
            return TimerId::STM32_TIM14;
#endif

        return TimerId::STM32_TIMER_ID_ERROR;
    }

    FORCE_INLINE float STM32Timer::GetDutyCycle(uint32_t channel) const
    {
        if (IS_TIM_CCX_INSTANCE(handle_->Instance, channel) == false) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Invalid channel: %u", channel);
            return 0.0f;
        }

        return __HAL_TIM_GET_COMPARE(handle_, channel) / static_cast<float>(__HAL_TIM_GET_AUTORELOAD(handle_));
    }

    STM32Timer::Channel::Channel(const char *name, STM32Timer &timer, uint32_t channel_number)
        : parent_(timer), channel_(channel_number)
    {
        APPKIT_RAISE_IF(nullptr == name, "PWM channel name must not be null");
        APPKIT_RAISE_IF_NOT(IS_TIM_CCX_INSTANCE(parent_.GetTIMHandle()->Instance, channel_number),
                            "Invalid timer channel number");

        // 初始化默认参数
        frequency = parent_.GetFrequency();
        duty_cycle = parent_.GetDutyCycle(channel_);

        // 创建设备文件
        APPKIT_RAISE_IF_NOT(Check(Block::RegisterDevice(name)), "Failed to register device");
    }

    ErrorCode STM32Timer::Channel::ApplyModify(PWM::ModifyState modify)
    {
        if (PWM::ModifyState::Frequency == modify)
        {
            return parent_.SetFrequency(frequency);
        }

        if (PWM::ModifyState::DutyCycle == modify)
        {
            return parent_.SetDutyCycle(channel_, duty_cycle);
        }

        if (PWM::ModifyState::Running == modify)
        {
            if (Block::running)
            {
                return HAL_OK == HAL_TIM_PWM_Start(parent_.GetTIMHandle(), channel_) ? ErrorCode::OK : ErrorCode::FAILED;
            }
            else
            {
                return HAL_OK == HAL_TIM_PWM_Stop(parent_.GetTIMHandle(), channel_) ? ErrorCode::OK : ErrorCode::FAILED;
            }
        }

        return ErrorCode::INVALID_ARG;
    }

    STM32Timer::STM32Timer(const char *name, TIM_HandleTypeDef &htim, bool as_clock)
        : Block(true), handle_(&htim), as_clock_(as_clock), is_32bit_(IS_TIM_32B_COUNTER_INSTANCE(htim.Instance))
    {
        APPKIT_RAISE_IF_NOT(nullptr != name, "Timer name must not be null");
        APPKIT_RAISE_IF_NOT(IS_TIM_INSTANCE(handle_->Instance), "Invalid timer instance");

        // 获取定时器时钟源频率
        tim_source_freq_ = 0;
#if defined(STM32F0) || defined(STM32G0)
        tim_source_freq_ = HAL_RCC_GetHCLKFreq();
#else
        if (
#if defined(TIM1)
            handle_->Instance == TIM1 ||
#endif
#if defined(TIM8)
            handle_->Instance == TIM8 ||
#endif
#if defined(TIM9)
            handle_->Instance == TIM9 ||
#endif
#if defined(TIM10)
            handle_->Instance == TIM10 ||
#endif
#if defined(TIM11)
            handle_->Instance == TIM11 ||
#endif
#if defined(TIM15)
            handle_->Instance == TIM15 ||
#endif
#if defined(TIM16)
            handle_->Instance == TIM16 ||
#endif
#if defined(TIM17)
            handle_->Instance == TIM17 ||
#endif
#if defined(TIM20)
            handle_->Instance == TIM20 ||
#endif
            false)
        {
            tim_source_freq_ = HAL_RCC_GetPCLK2Freq();
#if __has_include(<stm32f4xx_hal.h>)
            tim_source_freq_ *= 2;
#elif __has_include(<stm32f1xx_hal.h>)
            tim_source_freq_ *= 2;
#endif
        }
        else if (
#if defined(TIM2)
            handle_->Instance == TIM2 ||
#endif
#if defined(TIM3)
            handle_->Instance == TIM3 ||
#endif
#if defined(TIM4)
            handle_->Instance == TIM4 ||
#endif
#if defined(TIM5)
            handle_->Instance == TIM5 ||
#endif
#if defined(TIM6)
            handle_->Instance == TIM6 ||
#endif
#if defined(TIM7)
            handle_->Instance == TIM7 ||
#endif
#if defined(TIM12)
            handle_->Instance == TIM12 ||
#endif
#if defined(TIM13)
            handle_->Instance == TIM13 ||
#endif
#if defined(TIM14)
            handle_->Instance == TIM14 ||
#endif
            false)
        {
            tim_source_freq_ = HAL_RCC_GetPCLK1Freq();
#if __has_include(<stm32f4xx_hal.h>)
            tim_source_freq_ *= 2;
#elif __has_include(<stm32h7xx_hal.h>)
            tim_source_freq_ *= 2;
#endif
        }
#endif

        APPKIT_RAISE_IF_NOT(tim_source_freq_ != 0, "Failed to get timer clock frequency");

        tim_source_freq_ /= handle_->Init.Prescaler + 1;

        switch (__HAL_TIM_GET_CLOCKDIVISION(handle_))
        {
        case TIM_CLOCKDIVISION_DIV2:
            tim_source_freq_ /= 2;
            break;

        case TIM_CLOCKDIVISION_DIV4:
            tim_source_freq_ /= 4;
            break;

        default:
            break;
        }

        // 计算每周期计数值
        size_t per_period_count_ = (is_32bit_) ? UINT32_MAX : UINT16_MAX;

        // 判断是否已注册
        APPKIT_RAISE_IF(map_[std::to_underlying(GetTimerId(handle_))], "System clock already registered");

        // 判断是否注册为系统时钟
        if (as_clock_)
        {
            // 计算周期时间
            period_ = Duration::From<second>(static_cast<double>(per_period_count_) / tim_source_freq_);

            // 设置最小支持频率
            __HAL_TIM_SET_AUTORELOAD(handle_, per_period_count_ - 1);

#if USE_HAL_TIM_REGISTER_CALLBACKS == 1
            // 注册中断回调函数
            HAL_TIM_RegisterCallback(handle_, HAL_TIM_PERIOD_ELAPSED_CB_ID, TimerCallback);
#endif

            __HAL_TIM_ENABLE_IT(handle_, TIM_IT_UPDATE);

            // 创建设备文件
            APPKIT_RAISE_IF_NOT(Check(Block::RegisterDevice(name)), "Failed to register device");
        }
        else
        {
            // 初始化默认参数
            min_frequency_ = static_cast<float>(tim_source_freq_) / per_period_count_;
        }

        map_[std::to_underlying(GetTimerId(handle_))] = this;

        // 启动定时器
        if (HAL_TIM_Base_GetState(handle_) != HAL_TIM_STATE_BUSY)
        {
            if (HAL_TIM_Base_Start(handle_) != HAL_OK)
            {
                APPKIT_LOG_ERROR("Failed to start base timer");
            }
        }
    }

    STM32Timer::~STM32Timer() = default;
    
    __RAM_FUNC [[gnu::optimize("O3")]] TimePoint STM32Timer::Now() const
    {
        if (handle_ == nullptr || !as_clock_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("TimeBase not initialized");
            return TimePoint();
        }

        size_t cycle_count_pin; // 已过周期数
        double count;     // 当前计数值

        do
        {
            cycle_count_pin = cycle_count.load(std::memory_order_acquire);

            // 获取当前计数值
            count = __HAL_TIM_GET_COUNTER(handle_);
        } while (cycle_count.load(std::memory_order_acquire) != cycle_count_pin); // 确保在读取计数值期间未发生周期溢出

        // 计算时间戳
        return TimePoint(period_ * (cycle_count_pin + count / (is_32bit_ ? UINT32_MAX : UINT16_MAX)));
    }

    ErrorCode STM32Timer::SetFrequency(uint32_t frequency)
    {
        if (as_clock_) [[unlikely]]
        {
            return ErrorCode::NOT_SUPPORTED;
        }

        if (frequency < min_frequency_ || frequency > tim_source_freq_) [[unlikely]]
        {
            return ErrorCode::INVALID_ARG;
        }

        const auto scale = static_cast<float>(tim_source_freq_) / frequency;
        const auto period = static_cast<uint32_t>(scale) & (is_32bit_ ? UINT32_MAX : UINT16_MAX);

        __HAL_TIM_SET_AUTORELOAD(handle_, period);

        return ErrorCode::OK;
    }

    ErrorCode STM32Timer::SetDutyCycle(uint32_t channel, float duty_cycle)
    {
        if (IS_TIM_CCX_INSTANCE(handle_->Instance, channel) == false) [[unlikely]]
        {
            return ErrorCode::INVALID_ARG;
        }

        if (duty_cycle < 0.0f || duty_cycle > 1.0f) [[unlikely]]
        {
            return ErrorCode::INVALID_ARG;
        }

        uint32_t pulse = static_cast<uint32_t>((is_32bit_ ? UINT32_MAX : UINT16_MAX) * duty_cycle);
        __HAL_TIM_SET_COMPARE(handle_, channel, pulse);

        return ErrorCode::OK;
    }

    __RAM_FUNC [[gnu::optimize("O3")]] void STM32Timer::TimerCallback(TIM_HandleTypeDef *htim)
    {
        auto &instance_ = map_[std::to_underlying(GetTimerId(htim))];
        if (instance_ != nullptr) [[likely]]
        {
            instance_->cycle_count.fetch_add(1, std::memory_order_release);
        }
    }
}

#if USE_HAL_TIM_REGISTER_CALLBACKS == 0

extern "C"
{
    [[gnu::optimize("O3")]] void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
    {
        appkit::stm32::STM32Timer::TimerCallback(htim);
    }
}

#endif

#endif