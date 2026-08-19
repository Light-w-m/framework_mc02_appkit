#pragma once

#include <flash.hpp>

#include <vector>
#include <string>

namespace appkit
{
    class LinuxFlash : public Flash::Block
    {
    public:
        LinuxFlash(const char *flash_name, std::string file_path, size_t size, size_t min_write_size, size_t page_size, size_t sector_size);

        virtual ErrorCode Read(uint32_t offset, RawData data) override;
        virtual ErrorCode Write(uint32_t offset, ConstRawData data) override;
        virtual ErrorCode Erase(uint32_t offset, uint32_t size) override;

    private:
        std::string file_path_;
        std::vector<uint8_t> buffer_;
        uint32_t size_;

        ErrorCode SyncToFile();
    };
}