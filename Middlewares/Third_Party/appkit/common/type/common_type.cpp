/**
 * @file common_type.cpp
 * @brief 通用类型实现
 * @author dusk
 * @date 2025-12-26
 */
#include <common_type.hpp>

#include <common_mem.hpp>

namespace appkit
{
    RawData RawData::SubData(size_t offset, size_t new_size) const
    {
        if (offset >= size_) [[unlikely]]
        {
            return RawData{nullptr, 0};
        }

        if (offset + new_size > size_) [[unlikely]]
        {
            new_size = size_ - offset;
        }

        return RawData{static_cast<uint8_t *>(data_) + offset, new_size};
    }

    void RawData::CopyTo(const RawData &dest) const
    {
        if (nullptr != data_ && nullptr != dest.data_) [[likely]]
        {
            UNUSED(Memory::Copy(dest.data_, data_, dest.size_ < size_ ? dest.size_ : size_));
        }
    }

    void RawData::CopyTo_n(const RawData &dest, size_t n) const
    {
        if (nullptr != data_ && nullptr != dest.data_) [[likely]]
        {
            if (n > size_) [[unlikely]]
            {
                n = size_;
            }

            if (n > dest.size_) [[unlikely]]
            {
                n = dest.size_;
            }

            UNUSED(Memory::Copy(dest.data_, data_, n));
        }
    }

    void RawData::Fill(uint8_t value) const
    {
        if (nullptr != data_) [[likely]]
        {
            UNUSED(Memory::Set(data_, value, size_));
        }
    }

    ConstRawData ConstRawData::SubData(size_t offset, size_t new_size) const
    {
        if (offset >= size_) [[unlikely]]
        {
            return ConstRawData{nullptr, 0};
        }

        if (offset + new_size > size_) [[unlikely]]
        {
            new_size = size_ - offset;
        }

        return ConstRawData{static_cast<const uint8_t *>(data_) + offset, new_size};
    }

    void ConstRawData::CopyTo(const RawData &dest) const
    {
        if (nullptr != data_ && nullptr != dest.data_) [[likely]]
        {
            UNUSED(Memory::Copy(dest.data_, data_, dest.size_ < size_ ? dest.size_ : size_));
        }
    }

    void ConstRawData::CopyTo_n(const RawData &dest, size_t n) const
    {
        if (nullptr != data_ && nullptr != dest.data_) [[likely]]
        {
            if (n > size_) [[unlikely]]
            {
                n = size_;
            }

            if (n > dest.size_) [[unlikely]]
            {
                n = dest.size_;
            }

            UNUSED(Memory::Copy(dest.data_, data_, n));
        }
    }
}
