#pragma once

#include <common_type.hpp>
#include <hash.hpp>

#include <vector>

namespace appkit
{
    /**
     * @brief 容器元素条目结构体
     * @tparam T 元素类型
     * @note T不能为const类型
     */
    template <typename T>
        requires std::negation_v<std::is_const<T>>
    struct Entry final
    {
        RawData value;    ///< 元素引用
        const char *name; ///< 元素名称

        Entry(T &val, const char *name) noexcept
            : value(val), name(name)
        {
        }
    };

    /**
     * @brief 容器类
     */
    class Container final
    {
        DECL_COPY_DISABLE(Container)
        DECL_MOVE_DISABLE(Container)

    private:
        /**
         * @brief 容器元素数据结构体
         */
        struct EntryData final
        {
            uint64_t name_hash;     ///< 名称哈希值
            TypeInfo::ID type_hash; ///< 类型哈希值
            RawData data;           ///< 元素数据

            /**
             * @brief 构造函数
             * @tparam T 元素类型
             * @param entry 元素条目
             */
            template <typename T>
            EntryData(const Entry<T> &entry)
                : name_hash(math::FNV1a64(ConstRawData{entry.name, std::char_traits<char>::length(entry.name)})),
                  type_hash(TypeInfo::GetID<T>()),
                  data(RawData{entry.value})
            {
            }
        };

    public:
        /**
         * @brief 构造函数
         * @tparam T 元素类型
         * @param entries 元素列表
         */
        template <typename... Ts>
        Container(const Entry<Ts> &...entries)
        {
            entries_.reserve(sizeof...(Ts));
            (entries_.emplace_back(entries), ...);
        }

        /**
         * @brief 获取容器元素
         * @tparam T 元素类型
         * @param name 元素名称
         * @return 元素结果封装
         */
        template <typename T>
        Result<T *> Find(const char *name) noexcept
        {
            for (const uint64_t name_hash = math::FNV1a64(ConstRawData{name, std::char_traits<char>::length(name)});
                 const EntryData &entry : entries_)
            {
                if (entry.name_hash == name_hash)
                {
                    if (entry.type_hash == TypeInfo::GetID<T>())
                    {
                        RawData data = entry.data;
                        return Result<T *>::Ok(data.GetData<T>());
                    }
                    else
                    {
                        // 类型不匹配
                        return Result<T *>::Error(ErrorCode::FAILED);
                    }
                }
            }

            // 未找到
            return Result<T *>::Error(ErrorCode::NO_FOUND);
        }

    private:
        std::vector<EntryData> entries_; ///< 容器元素列表
    };
} // namespace appkit
