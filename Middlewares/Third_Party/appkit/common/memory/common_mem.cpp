#include <common_mem.hpp>

#include <cstring>

namespace appkit
{
    [[using gnu: optimize("O3")]] Result<bool> Memory::Compare(const void *buf1, const void *buf2, size_t size)
    {
        if (buf1 == nullptr || buf2 == nullptr || size == 0) [[unlikely]]
        {
            return Result<bool>::Error(ErrorCode::INVALID_ARG);
        }

        const int cmp_result = std::memcmp(buf1, buf2, size);
        return Result<bool>::Ok(cmp_result == 0);
    }
} // namespace appkit