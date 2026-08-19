#include <power.hpp>

#include <logger.hpp>

namespace appkit
{
    ErrorCode PowerManager::Open(const char *name)
    {
        if (nullptr == name) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Invalid device name");
            return ErrorCode::INVALID_ARG;
        }

        if (nullptr != block_) [[unlikely]]
        {
            APPKIT_LOG_ERROR("Device already open");
            return ErrorCode::FAILED;
        }

        return Block::GetDeviceBlock(name, &block_);
    }

    void PowerManager::Reset() const
    {
        APPKIT_RAISE_IF_NOT(nullptr != block_, "Device not opened");
        block_->ChangeStatus(Block::Status::Reset);
    }

    void PowerManager::Shutdown() const
    {
        APPKIT_RAISE_IF_NOT(nullptr != block_, "Device not opened");
        block_->ChangeStatus(Block::Status::Shutdown);
    }

    void PowerManager::JumpToBootloader() const
    {
        APPKIT_RAISE_IF_NOT(nullptr != block_, "Device not opened");
        block_->ChangeStatus(Block::Status::JumpToBootloader);
    }
} // namespace appkit