#include <pwm.hpp>

#include <logger.hpp>

namespace appkit
{
    ErrorCode PWM::Open(const char *name)
    {
        if (nullptr == name) [[unlikely]]
        {
            return ErrorCode::INVALID_ARG;
        }

        if (nullptr != block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Device already open");
            return ErrorCode::FAILED;
        }

        return Block::GetDeviceBlock(name, &block_);
    }

    ErrorCode PWM::Enable() const
    {
        if (nullptr == block_) [[unlikely]]
        {
            APPKIT_RAISE("Device not opened");
            return ErrorCode::NOT_SUPPORTED;
        }

        [[maybe_unused]] osal::LockGuard lock{block_->mutex};
        block_->running = true;
        return block_->ApplyModify(ModifyState::Running);
    }

    ErrorCode PWM::Disable() const
    {
        if (nullptr == block_) [[unlikely]]
        {
            APPKIT_RAISE("Device not opened");
            return ErrorCode::NOT_SUPPORTED;
        }

        [[maybe_unused]] osal::LockGuard lock{block_->mutex};
        block_->running = false;
        return block_->ApplyModify(ModifyState::Running);
    }

    ErrorCode PWM::SetFrequency(const float frequency) const
    {
        if (nullptr == block_) [[unlikely]]
        {
            APPKIT_RAISE("Device not opened");
            return ErrorCode::NOT_SUPPORTED;
        }

        if (frequency <= 0) [[unlikely]]
        {
            APPKIT_LOG_WARNING("%p set freq <%f> <= 0", &block_, frequency);
            return ErrorCode::OUT_OF_RANGE;
        }

        [[maybe_unused]] osal::LockGuard lock{block_->mutex};
        block_->frequency = frequency;

        return block_->ApplyModify(ModifyState::Frequency);
    }

    ErrorCode PWM::SetDutyCycle(const float duty_cycle) const
    {
        if (nullptr == block_) [[unlikely]]
        {
            APPKIT_RAISE("Device not opened");
            return ErrorCode::NOT_SUPPORTED;
        }

        if (duty_cycle < 0) [[unlikely]]
        {
            APPKIT_LOG_WARNING("%p set duty_cycle <%f> < 0", &block_, duty_cycle);
            return ErrorCode::OUT_OF_RANGE;
        }

        if (duty_cycle > 1) [[unlikely]]
        {
            APPKIT_LOG_WARNING("%p set duty_cycle <%f> > 1", &block_, duty_cycle);
            return ErrorCode::OUT_OF_RANGE;
        }

        [[maybe_unused]] osal::LockGuard lock{block_->mutex};
        block_->duty_cycle = duty_cycle;

        return block_->ApplyModify(ModifyState::DutyCycle);
    }
}
