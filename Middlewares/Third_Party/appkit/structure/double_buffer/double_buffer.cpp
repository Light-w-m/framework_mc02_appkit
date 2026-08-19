#include <double_buffer.hpp>

#include <cstring>

namespace appkit
{
    DoubleBuffer::DoubleBuffer(RawData &raw_data) : capacity_(raw_data.GetSize() / 2)
    {
        const auto buffer_ptr = raw_data.GetData<std::byte>();
        buffer_[0] = {buffer_ptr, capacity_};
        buffer_[1] = {buffer_ptr + capacity_, capacity_};
    }

    bool DoubleBuffer::FillActiveBuffer(ConstRawData buffer)
    {
        if (buffer.GetSize() > capacity_)
        {
            return false;
        }

        buffer.CopyTo(RawData{buffer_[active_]});
        return true;
    }

    bool DoubleBuffer::FillPendingBuffer(ConstRawData buffer)
    {
        // 如果缓存剩余空间不足，则返回false
        if (buffer.GetSize() > capacity_ - pending_length_)
        {
            return false;
        }

        buffer.CopyTo(RawData{buffer_[1 - active_].GetData<uint8_t>() + pending_length_,
                              capacity_ - pending_length_});
        pending_length_ += buffer.GetSize();
        pending_valid_ = true;
        return true;
    }

    void DoubleBuffer::Switch(const bool force)
    {
        // 如果强制切换或者待切换缓冲区有效，则切换缓冲区
        if (force || pending_valid_)
        {
            active_ ^= 1;
            pending_valid_ = false;
            pending_length_ = 0;
        }
    }

    void DoubleBuffer::SetPendingUsed(size_t length)
    {
        pending_length_ = length;
        pending_valid_ = pending_length_ > 0;
    }
}
