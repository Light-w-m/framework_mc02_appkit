#include <spi.hpp>

#include <logger.hpp>

namespace appkit
{
    ErrorCode SPI::Open(const char *name)
    {
        if (nullptr == name) [[unlikely]]
        {
            return ErrorCode::INVALID_ARG;
        }

        if (nullptr != block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Device %s already open", name);
            return ErrorCode::FAILED;
        }

        return Block::GetDeviceBlock(name, &block_);
    }

    ErrorCode SPI::Read(RawData data, ReadOperation &operation) const
    {
        if (block_ == nullptr) [[unlikely]]
        {
            return ErrorCode::NOT_SUPPORTED;
        }

        return block_->Read(data, operation);
    }

    ErrorCode SPI::Write(ConstRawData data, WriteOperation &operation) const
    {
        if (block_ == nullptr) [[unlikely]]
        {
            return ErrorCode::NOT_SUPPORTED;
        }

        return block_->Write(data, operation);
    }

    ErrorCode SPI::ReadAndWrite(ConstRawData tx_data, RawData rx_data, ReadOperation &operation) const
    {
        if (block_ == nullptr) [[unlikely]]
        {
            return ErrorCode::NOT_SUPPORTED;
        }

        return block_->ReadAndWrite(tx_data, rx_data, operation);
    }
} // namespace appkit
