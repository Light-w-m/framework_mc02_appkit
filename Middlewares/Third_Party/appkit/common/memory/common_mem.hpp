#pragma once

#include <common_type.hpp>

namespace appkit
{
    class Memory final
    {
    public:
        /**
         * @brief 内存拷贝
         * @param dst 目标地址
         * @param src 源地址
         * @param size 拷贝大小
         * @return 错误码
         */
        static ErrorCode Copy(void *dst, const void *src, size_t size);

        /**
         * @brief 内存设置
         * @param dst 目标地址
         * @param value 设置值
         * @param size 设置大小
         * @return 错误码
         */
        static ErrorCode Set(void *dst, uint8_t value, size_t size);

        /**
         * @brief 内存比较
         * @param buf1 内存块1地址
         * @param buf2 内存块2地址
         * @param size 比较大小
         * @param equal 是否相等
         * @return 比较结果
         */
        static Result<bool> Compare(const void *buf1, const void *buf2, size_t size);
    };

    [[using gnu: optimize("O3")]] inline ErrorCode Memory::Copy(void *dst, const void *src, size_t size)
    {
        if (dst == nullptr || src == nullptr || size == 0) [[unlikely]]
        {
            return ErrorCode::INVALID_ARG;
        }

        uint8_t *d = static_cast<uint8_t *>(dst);
        const uint8_t *s = static_cast<const uint8_t *>(src);

        const uintptr_t dst_offset = reinterpret_cast<uintptr_t>(dst) & (SYSTEM_ALIGN_SIZE - 1);
        const uintptr_t src_offset = reinterpret_cast<uintptr_t>(src) & (SYSTEM_ALIGN_SIZE - 1);

        // 判断是否有相同偏移
        if (dst_offset == src_offset)
        {
            // 对齐到缓存行
            if (dst_offset != 0)
            {
                const size_t align_size = SYSTEM_ALIGN_SIZE - dst_offset;
                for (size_t i = 0; i < align_size; ++i)
                {
                    *d++ = *s++;
                }
                size -= align_size;
                if (size == 0)
                {
                    return ErrorCode::OK;
                }
            }

            uintptr_t *d_aligned = reinterpret_cast<uintptr_t *>(d);
            const uintptr_t *s_aligned = reinterpret_cast<const uintptr_t *>(s);

            // 按缓存行大小拷贝
            while (size >= SYSTEM_CACHE_LINE_SIZE)
            {
                d_aligned[0] = s_aligned[0];
                d_aligned[1] = s_aligned[1];
                d_aligned[2] = s_aligned[2];
                d_aligned[3] = s_aligned[3];
                d_aligned[4] = s_aligned[4];
                d_aligned[5] = s_aligned[5];
                d_aligned[6] = s_aligned[6];
                d_aligned[7] = s_aligned[7];

                d_aligned += 8;
                s_aligned += 8;
                size -= SYSTEM_CACHE_LINE_SIZE;
            }

            // 按对齐大小拷贝剩余部分
            while (size >= SYSTEM_ALIGN_SIZE)
            {
                *d_aligned++ = *s_aligned++;
                size -= SYSTEM_ALIGN_SIZE;
            }

            d = reinterpret_cast<uint8_t *>(d_aligned);
            s = reinterpret_cast<const uint8_t *>(s_aligned);
        }
        else
        {
            uintptr_t addr_diff = dst_offset > src_offset
                                      ? dst_offset - src_offset
                                      : src_offset - dst_offset;
#if SYSTEM_ALIGN_SIZE == 8
            // 尝试4字节对齐复制
            if ((addr_diff & (4 - 1)) == 0)
            {
                while (reinterpret_cast<uintptr_t>(d) & 3)
                {
                    *d++ = *s++;
                    --size;
                }

                uint32_t *d_aligned = reinterpret_cast<uint32_t *>(d);
                const uint32_t *s_aligned = reinterpret_cast<const uint32_t *>(s);

                while (size >= 32)
                {
                    d_aligned[0] = s_aligned[0];
                    d_aligned[1] = s_aligned[1];
                    d_aligned[2] = s_aligned[2];
                    d_aligned[3] = s_aligned[3];
                    d_aligned[4] = s_aligned[4];
                    d_aligned[5] = s_aligned[5];
                    d_aligned[6] = s_aligned[6];
                    d_aligned[7] = s_aligned[7];

                    d_aligned += 8;
                    s_aligned += 8;
                    size -= 32;
                }

                // 按对齐大小拷贝剩余部分
                while (size >= 4)
                {
                    *d_aligned++ = *s_aligned++;
                    size -= 4;
                }

                d = reinterpret_cast<uint8_t *>(d_aligned);
                s = reinterpret_cast<const uint8_t *>(s_aligned);
            }
            else
#endif

                // 尝试2字节对齐复制
                if ((addr_diff & (2 - 1)) == 0)
                {
                    if (reinterpret_cast<uintptr_t>(d) & 1)
                    {
                        *d++ = *s++;
                        --size;
                    }

                    uint16_t *d_aligned = reinterpret_cast<uint16_t *>(d);
                    const uint16_t *s_aligned = reinterpret_cast<const uint16_t *>(s);

                    while (size >= 16)
                    {
                        d_aligned[0] = s_aligned[0];
                        d_aligned[1] = s_aligned[1];
                        d_aligned[2] = s_aligned[2];
                        d_aligned[3] = s_aligned[3];
                        d_aligned[4] = s_aligned[4];
                        d_aligned[5] = s_aligned[5];
                        d_aligned[6] = s_aligned[6];
                        d_aligned[7] = s_aligned[7];

                        d_aligned += 8;
                        s_aligned += 8;
                        size -= 16;
                    }

                    // 按对齐大小拷贝剩余部分
                    while (size >= 2)
                    {
                        *d_aligned++ = *s_aligned++;
                        size -= 2;
                    }

                    d = reinterpret_cast<uint8_t *>(d_aligned);
                    s = reinterpret_cast<const uint8_t *>(s_aligned);
                }

            // 否则按字节拷贝
        }

        // 拷贝剩余字节
        while (size > 0)
        {
            *d++ = *s++;
            --size;
        }

        return ErrorCode::OK;
    }

    [[using gnu: optimize("O3")]] inline ErrorCode Memory::Set(void *dst, uint8_t value, size_t size)
    {
        if (dst == nullptr || size == 0) [[unlikely]]
        {
            return ErrorCode::INVALID_ARG;
        }

        uint8_t *d = static_cast<uint8_t *>(dst);
        const uintptr_t dst_offset = reinterpret_cast<uintptr_t>(dst) & (SYSTEM_ALIGN_SIZE - 1);

        // 对齐到缓存行
        if (dst_offset != 0)
        {
            size_t align_size = SYSTEM_ALIGN_SIZE - dst_offset;
            if (size < align_size)
            {
                align_size = size;
            }
            for (size_t i = 0; i < align_size; ++i)
            {
                *d++ = value;
            }
            size -= align_size;
            if (size == 0)
            {
                return ErrorCode::OK;
            }
        }

        // 按缓存行大小拷贝
        {
            uintptr_t aligned_value = static_cast<uintptr_t>(value);
            aligned_value |= aligned_value << 8;
            aligned_value |= aligned_value << 16;
#if SYSTEM_ALIGN_SIZE == 8
            aligned_value |= aligned_value << 32;
#endif
            uintptr_t *d_aligned = reinterpret_cast<uintptr_t *>(d);
            while (size >= SYSTEM_CACHE_LINE_SIZE)
            {
                d_aligned[0] = aligned_value;
                d_aligned[1] = aligned_value;
                d_aligned[2] = aligned_value;
                d_aligned[3] = aligned_value;
                d_aligned[4] = aligned_value;
                d_aligned[5] = aligned_value;
                d_aligned[6] = aligned_value;
                d_aligned[7] = aligned_value;

                d_aligned += 8;
                size -= SYSTEM_CACHE_LINE_SIZE;
            }

            // 按对齐大小拷贝
            while (size >= SYSTEM_ALIGN_SIZE)
            {
                *d_aligned++ = aligned_value;
                size -= SYSTEM_ALIGN_SIZE;
            }

            d = reinterpret_cast<uint8_t *>(d_aligned);
        }

        // 拷贝剩余字节
        while (size > 0)
        {
            *d++ = value;
            --size;
        }

        return ErrorCode::OK;
    }
} // namespace appkit
