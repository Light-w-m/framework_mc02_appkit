#include <linux_flash.hpp>

#include <logger.hpp>
#include <fstream>
#include <filesystem>

namespace appkit
{
    LinuxFlash::LinuxFlash(const char *flash_name, std::string file_path, size_t size, size_t min_write_size, size_t page_size, size_t sector_size)
        : file_path_(std::move(file_path)), size_(static_cast<uint32_t>(size))
    {
        bool is_exists = false;
        if (std::filesystem::exists(file_path_))
        {
            APPKIT_LOG_INFO("Flash file %s exists, loading...", file_path_.c_str());
            is_exists = true;
        }
        else
        {
            APPKIT_LOG_INFO("Flash file %s does not exist, creating...", file_path_.c_str());
            std::ofstream create_file(file_path_, std::ios::binary);
            if (!create_file.is_open())
            {
                APPKIT_LOG_ERROR("Failed to create flash file: %s", file_path_.c_str());
                return;
            }
        }

        std::ifstream file(file_path_, std::ios::binary);
        if (!file.is_open())
        {
            APPKIT_LOG_ERROR("Failed to open flash file: %s", file_path_.c_str());
            return;
        }

        // 读取文件内容到缓冲区
        buffer_.resize(size_);
        file.read(reinterpret_cast<char *>(buffer_.data()), size_);

        if (!is_exists)
        {
            APPKIT_LOG_INFO("Flash file %s does not init, formatting...", file_path_.c_str());
            std::fill(buffer_.begin(), buffer_.end(), 0xFF);
            SyncToFile();
        }

        // 初始化Flash属性
        Block::size = static_cast<uint32_t>(size);
        Block::min_write_size = static_cast<uint32_t>(min_write_size);
        Block::page_size = static_cast<uint32_t>(page_size);
        Block::sector_size = static_cast<uint32_t>(sector_size);

        APPKIT_RAISE_IF_NOT(Check(RegisterDevice(flash_name)), "Failed to register flash device");
    }

    ErrorCode LinuxFlash::Read(uint32_t offset, RawData data)
    {
        if (offset + data.GetSize() > size_)
        {
            APPKIT_LOG_ERROR("Read out of bounds: offset=%u, size=%zu", offset, data.GetSize());
            return ErrorCode::OUT_OF_RANGE;
        }

        ConstRawData{buffer_.data() + offset, data.GetSize()}.CopyTo(data);
        return ErrorCode::OK;
    }

    ErrorCode LinuxFlash::Write(uint32_t offset, ConstRawData data)
    {
        if (offset + data.GetSize() > size_)
        {
            APPKIT_LOG_ERROR("Write out of bounds: offset=%u, size=%zu", offset, data.GetSize());
            return ErrorCode::OUT_OF_RANGE;
        }

        data.CopyTo(RawData{buffer_.data() + offset, data.GetSize()});
        return SyncToFile();
    }

    ErrorCode LinuxFlash::Erase(uint32_t offset, uint32_t size)
    {
        if (offset + size > size_)
        {
            APPKIT_LOG_ERROR("Erase out of bounds: offset=%u, size=%u", offset, size);
            return ErrorCode::OUT_OF_RANGE;
        }

        std::fill(buffer_.begin() + offset, buffer_.begin() + offset + size, 0xFF);
        return SyncToFile();
    }

    ErrorCode LinuxFlash::SyncToFile()
    {
        std::ofstream file(file_path_, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            APPKIT_LOG_ERROR("Failed to open flash file for writing: %s", file_path_.c_str());
            return ErrorCode::FAILED;
        }

        file.write(reinterpret_cast<const char *>(buffer_.data()), buffer_.size());
        return ErrorCode::OK;
    }
}