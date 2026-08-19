#pragma once

#include <common_type.hpp>

namespace appkit::math
{
    /**
     * @brief FNV-1a 64位哈希函数
     * @param str 输入字符串
     * @return 64位哈希值
     */
    static inline constexpr uint64_t FNV1a64(const ConstRawData str) noexcept
    {
        static constexpr uint64_t fnv_prime = 1099511628211ULL;
        static constexpr uint64_t fnv_offset_basis = 14695981039346656037ULL;

        uint64_t hash = fnv_offset_basis;
        for (size_t i = 0; i < str.GetSize(); ++i)
        {
            hash ^= static_cast<uint64_t>(str.GetData<uint8_t>()[i]);
            hash *= fnv_prime;
        }
        return hash;
    }

    /**
     * @brief FNV-1a 32位哈希函数
     * @param str 输入字符串
     * @return 32位哈希值
     */
    static inline constexpr uint32_t FNV1a32(const ConstRawData str) noexcept
    {
        static constexpr uint32_t fnv_prime = 16777619U;
        static constexpr uint32_t fnv_offset_basis = 2166136261U;

        uint32_t hash = fnv_offset_basis;
        for (size_t i = 0; i < str.GetSize(); ++i)
        {
            hash ^= static_cast<uint32_t>(str.GetData<uint8_t>()[i]);
            hash *= fnv_prime;
        }
        return hash;
    }
}