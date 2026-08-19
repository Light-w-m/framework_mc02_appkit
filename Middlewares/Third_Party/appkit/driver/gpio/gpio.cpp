#include <gpio.hpp>

#include <logger.hpp>

namespace appkit
{
    GPIO::~GPIO()
    {
        if (block_)
        {
            if (is_register_)
            {
                block_->cb_list.Delete(cb_);
            }
        }
    }

    ErrorCode GPIO::Open(const char *name)
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

    bool GPIO::Read() const
    {
        if (nullptr == block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Device not opened");
            return false;
        }

        return block_->Read();
    }

    ErrorCode GPIO::Write(bool value)
    {
        if (nullptr == block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Device not opened");
            return ErrorCode::NOT_SUPPORTED;
        }

        return block_->Write(value);
    }

    ErrorCode GPIO::SetInterrupt(bool enable)
    {
        if (nullptr == block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Device not opened");
            return ErrorCode::NOT_SUPPORTED;
        }

        return block_->SetInterrupt(enable);
    }

    ErrorCode GPIO::RegisterCallback(Callback cb)
    {
        if (nullptr == block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Device not opened");
            return ErrorCode::NOT_SUPPORTED;
        }

        if (!is_register_) [[likely]]
        {
            is_register_ = true;
            block_->cb_list.Add(cb_);
        }

        cb_ = cb;
        return ErrorCode::OK;
    }
} // namespace appkit
