#include <uart.hpp>

#include <logger.hpp>

namespace appkit
{
    ErrorCode Uart::Open(const char *name)
    {
        if (nullptr == name) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Invalid device name");
            return ErrorCode::INVALID_ARG;
        }

        if (nullptr != block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Device %s already open", name);
            return ErrorCode::FAILED;
        }

        return Block::GetDeviceBlock(name, &block_);
    }

    ErrorCode Uart::SetConfig(const Config &config) const
    {
        if (nullptr == block_) [[unlikely]]
        {
            return ErrorCode::FAILED;
        }
        return block_->SetConfig(config);
    }

    ErrorCode Uart::Read(const RawData &data, ReadOperation &operation) const
    {
        if (nullptr == block_->read) [[unlikely]]
        {
            return ErrorCode::NOT_SUPPORTED;
        }
        return (*block_->read)(data, operation);
    }

    ErrorCode Uart::Write(const ConstRawData &data, WriteOperation &operation) const
    {
        if (nullptr == block_->write) [[unlikely]]
        {
            return ErrorCode::NOT_SUPPORTED;
        }
        return (*block_->write)(data, operation);
    }
}
