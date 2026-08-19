/**
 * @file hash_string.hpp
 * @brief 哈希字符串定义
 * @author dusk
 */
#pragma once

#include <macros.hpp>
#include <hash.hpp>

#include <string_view>

namespace appkit
{
    template <size_t Capacity>
    class HashString final
    {
    public:
        using HashType = uint64_t;

    private:
        /**
         * @brief 生成字符串的哈希值
         * @param str 字符串
         * @return 字符串的哈希值
         * @note 该函数使用FNV-1a算法生成哈希值
         */
        FORCE_INLINE static constexpr HashType GenerateHash(const ConstRawData str)
        {
            return math::FNV1a64(str);
        }

        static const constinit inline auto emptyHashValue = GenerateHash({nullptr, 0}); ///< 空字符串的哈希值

        char buffer_[Capacity + 1]{}; ///< 字符串缓冲区
        size_t length_{};             ///< 字符串长度

        RawData strData_{buffer_, Capacity + 1}; ///< 字符串原始数据
        uint64_t hashValue_{emptyHashValue};     ///< 字符串哈希值

    public:
        /**
         * @brief 构造函数
         * @param str 字符串
         */
        constexpr HashString(const std::string_view str = "")
            : length_(std::min(str.length(), Capacity))
        {
            std::char_traits<char>::copy(buffer_, str.data(), length_);
            std::char_traits<char>::assign(buffer_[length_], '\0');

            hashValue_ = GenerateHash({buffer_, length_});
        }

        /**
         * @brief 析构函数
         */
        ~HashString() = default;

        /**
         * @brief 拷贝构造函数
         * @param other 另一个HashString
         */
        constexpr HashString(const HashString &other)
            : length_(other.length_), hashValue_(other.hashValue_)
        {
            if consteval
            {
                for (size_t i = 0; i < length_ + 1; ++i)
                {
                    buffer_[i] = other.buffer_[i];
                }
            }
            else
            {
                other.strData_.CopyTo_n(strData_, length_ + 1);
            }
        }

        /**
         * @brief 拷贝赋值运算符
         * @param other 另一个HashString
         * @return 赋值后的HashString
         */
        constexpr HashString &operator=(const HashString &other)
        {
            if (this != &other)
            {
                length_ = other.length_;
                hashValue_ = other.hashValue_;

                if consteval
                {
                    for (size_t i = 0; i < length_ + 1; ++i)
                    {
                        buffer_[i] = other.buffer_[i];
                    }
                }
                else
                {
                    other.strData_.CopyTo_n(strData_, length_ + 1);
                }
            }
            return *this;
        }

        /**
         * @brief 移动构造函数
         * @param other 另一个HashString
         */
        constexpr HashString(HashString &&other) noexcept
            : length_(other.length_), hashValue_(other.hashValue_)
        {
            if consteval
            {
                for (size_t i = 0; i < length_ + 1; ++i)
                {
                    buffer_[i] = other.buffer_[i];
                }
            }
            else
            {
                other.strData_.CopyTo_n(strData_, length_ + 1);
            }

            other.length_ = 0;
            other.hashValue_ = emptyHashValue;

            std::char_traits<char>::assign(other.buffer_[0], '\0');
        }

        /**
         * @brief 移动赋值运算符
         * @param other 另一个HashString
         * @return 赋值后的HashString
         */
        constexpr HashString &operator=(HashString &&other) noexcept
        {
            if (this != &other)
            {
                length_ = other.length_;
                hashValue_ = other.hashValue_;

                if consteval
                {
                    for (size_t i = 0; i < length_ + 1; ++i)
                    {
                        buffer_[i] = other.buffer_[i];
                    }
                }
                else
                {
                    other.strData_.CopyTo_n(strData_, length_ + 1);
                }

                other.length_ = 0;
                other.hashValue_ = emptyHashValue;

                std::char_traits<char>::assign(other.buffer_[0], '\0');
            }
            return *this;
        }

        /**
         * @brief 赋值运算符
         * @param str 字符串
         * @return 赋值后的HashString
         */
        constexpr HashString &operator=(const std::string_view str)
        {
            length_ = std::min(str.length(), Capacity);
            std::char_traits<char>::copy(buffer_, str.data(), length_);
            std::char_traits<char>::assign(buffer_[length_], '\0');

            hashValue_ = math::FNV1a64({buffer_, length_});
            return *this;
        }

        /**
         * @brief 赋值运算符
         * @param str 字符串
         * @return 赋值后的HashString
         */
        template <size_t N>
        constexpr HashString &operator=(const char (&str)[N])
        {
            length_ = std::min(N, Capacity);
            std::char_traits<char>::copy(buffer_, str, length_);
            std::char_traits<char>::assign(buffer_[length_], '\0');

            hashValue_ = math::FNV1a64({buffer_, length_});
            return *this;
        }

        /**
         * @brief 获取原始数据
         * @return 原始数据
         */
        [[nodiscard]] FORCE_INLINE constexpr ConstRawData GetRawData() const
        {
            return strData_;
        }

        /**
         * @brief 获取字符串
         * @return 字符串
         */
        [[nodiscard]] FORCE_INLINE constexpr const char *GetString() const
        {
            return strData_.GetData<char>();
        }

        /**
         * @brief 获取字符串长度
         * @return 字符串长度
         */
        [[nodiscard]] FORCE_INLINE constexpr size_t GetLength() const
        {
            return length_;
        }

        /**
         * @brief 获取字符串容量
         * @return 字符串容量
         */
        [[nodiscard]] FORCE_INLINE constexpr size_t GetCapacity() const
        {
            return Capacity;
        }

        /**
         * @brief 获取字符串哈希值
         * @return 字符串哈希值
         */
        [[nodiscard]] FORCE_INLINE constexpr uint64_t GetHashValue() const
        {
            return hashValue_;
        }

        /**
         * @brief 比较两个HashString是否相等
         * @param other 另一个HashString
         * @return 是否相等
         */
        [[nodiscard]] constexpr std::strong_ordering operator<=>(const HashString &other) const
        {
            if (hashValue_ != other.hashValue_) [[likely]]
                return hashValue_ <=> other.hashValue_;

            if (length_ != other.length_)
                return length_ <=> other.length_;

            return std::char_traits<char>::compare(buffer_, other.buffer_, length_) <=> 0;
        }

        /**
         * @brief 创建字符串的哈希值
         * @param str 字符串
         * @return 字符串的哈希值
         */
        [[nodiscard]] FORCE_INLINE static constexpr HashType CreateHash(const std::string_view str)
        {
            return GenerateHash({str.data(), std::min(str.length(), Capacity)});
        }
    };
} // namespace LIB_NAMESPACE