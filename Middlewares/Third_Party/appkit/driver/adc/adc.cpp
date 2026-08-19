#include <adc.hpp>

#include <logger.hpp>

namespace appkit
{
    ErrorCode ADC::Open(const char* name)
    {
        if (nullptr == name) {
            return ErrorCode::INVALID_ARG;
        }

        if (nullptr != block_) {
            APPKIT_LOG_ERROR("Device already open");
            return ErrorCode::FAILED;
        }

        return Block::GetDeviceBlock(name, &block_);
    }

    Result<float> ADC::Read() const
    {
        if (nullptr == block_)
        {
            APPKIT_LOG_ERROR("ADC device not opened");
            return Result<float>::Error(ErrorCode::NOT_SUPPORTED);
        }

        return block_->Read();
    }
} // namespace appkit