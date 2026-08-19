#include <flash.hpp>

#include <logger.hpp>

namespace appkit
{
    ErrorCode Flash::Open(const char *name)
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

    ErrorCode Flash::Read(uint32_t offset, RawData data)
    {
        if (nullptr == block_) [[unlikely]]
        {
            APPKIT_RAISE("Device not open");
            return ErrorCode::NOT_SUPPORTED;
        }

        if (offset + data.GetSize() > block_->size) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Read out of range");
            return ErrorCode::OUT_OF_RANGE;
        }

        return block_->Read(offset, data);
    }

    ErrorCode Flash::Write(uint32_t offset, ConstRawData data)
    {
        if (nullptr == block_) [[unlikely]]
        {
            APPKIT_RAISE("Device not open");
            return ErrorCode::NOT_SUPPORTED;
        }

        if (offset + data.GetSize() > block_->size) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Write out of range");
            return ErrorCode::OUT_OF_RANGE;
        }

        return block_->Write(offset, data);
    }

    ErrorCode Flash::Erase(uint32_t offset, uint32_t size)
    {
        if (nullptr == block_) [[unlikely]]
        {
            APPKIT_RAISE("Device not open");
            return ErrorCode::NOT_SUPPORTED;
        }

        if (offset + size > block_->size) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Erase out of range");
            return ErrorCode::OUT_OF_RANGE;
        }

        return block_->Erase(offset, size);
    }

    uint32_t Flash::GetSize() const
    {
        if (nullptr == block_) [[unlikely]]
        {
            return 0;
        }

        return block_->size;
    }

    uint32_t Flash::GetMinWriteSize() const
    {
        if (nullptr == block_) [[unlikely]]
        {
            return 0;
        }

        return block_->min_write_size;
    }

    uint32_t Flash::GetPageSize() const
    {
        if (nullptr == block_) [[unlikely]]
        {
            return 0;
        }

        return block_->page_size;
    }

    uint32_t Flash::GetSectorSize() const
    {
        if (nullptr == block_) [[unlikely]]
        {
            return 0;
        }

        return block_->sector_size;
    }
}
