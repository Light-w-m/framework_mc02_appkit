/**
 * @file lockfree_list.h
 * @brief 无锁链表定义
 * @author dusk
 */
#pragma once

#include <common_assert.hpp>
#include <common_type.hpp>

#include <atomic>
#include <utility>

namespace appkit
{
    /**
     * @brief 无锁链表
     */
    class alignas(SYSTEM_CACHE_LINE_SIZE) LockFreeList
    {
    private:
        class NodeBase
        {
            // 保证原子操作的无锁性
            static_assert(std::atomic<NodeBase *>::is_always_lock_free);

            const size_t size_;                                                     ///< 节点数据类型
            alignas(SYSTEM_CACHE_LINE_SIZE) std::atomic<NodeBase *> next_{nullptr}; ///< 下一个节点

            friend class LockFreeList;

            /**
             * @brief 构造函数
             * @param tag 数据类型标签
             */
            constexpr explicit NodeBase(size_t size) : size_(size)
            {
            }

            /**
             * @brief 析构函数
             */
            ~NodeBase();

            /**
             * @brief 获取自身引用
             * @return 自身引用
             */
            FORCE_INLINE NodeBase &GetSelf()
            {
                return *this;
            }
        };

    public:
        template <typename DType>
        class Node : public NodeBase
        {
            DType data_{};

        public:
            /**
             * @brief 默认构造函数
             */
            constexpr Node() : NodeBase(sizeof(DType))
            {
            }

            /**
             * @brief 原地构建数据
             * @tparam AType 构造参数类型
             * @param arg 构造参数
             */
            template <typename... Args>
            constexpr explicit Node(Args &&...arg) : NodeBase(sizeof(DType)), data_{std::forward<Args>(arg)...}
            {
            }

            /**
             * @brief 复制数据
             * @param data 要存入的数据
             */
            constexpr explicit Node(const DType &data) : NodeBase(sizeof(DType)), data_(data)
            {
            }

            /**
             * @brief 复制数据
             * @param data 要存入的数据
             * @return *this
             */
            FORCE_INLINE constexpr Node &operator=(const DType &data)
            {
                this->data_ = data;
                return *this;
            }

            /**
             * @brief 获取数据引用
             * @return 数据引用
             */
            FORCE_INLINE DType &GetData()
            {
                return data_;
            }

            /**
             * @brief 获取数据常量引用
             * @return 数据常量引用
             */
            FORCE_INLINE std::add_const_t<DType> &GetData() const
            {
                return std::as_const(data_);
            }

            /**
             * @brief 获取数据引用
             * @return 数据引用
             */
            FORCE_INLINE DType &operator*()
            {
                return data_;
            }

            /**
             * @brief 获取数据常量引用
             * @return 数据常量引用
             */
            FORCE_INLINE std::add_const_t<DType> &operator*() const
            {
                return std::as_const(data_);
            }

            /**
             * @brief 获取数据指针
             * @return 数据指针
             */
            FORCE_INLINE DType *operator->()
            {
                return std::addressof(data_);
            }

            /**
             * @brief 获取数据常量指针
             * @return 数据常量指针
             */
            FORCE_INLINE std::add_const_t<DType> *operator->() const
            {
                return std::addressof(std::as_const(data_));
            }
        };

    private:
        alignas(SYSTEM_CACHE_LINE_SIZE) std::atomic<NodeBase *> head_{nullptr}; ///< 头节点
        alignas(SYSTEM_CACHE_LINE_SIZE) std::atomic<size_t> size_{0};           ///< 链表大小

    public:
        /**
         * @brief 构造函数
         */
        constexpr LockFreeList()
        {
        }

        /**
         * @brief 析构函数
         */
        ~LockFreeList();

        /**
         * @brief 添加节点到链表头
         * @param data 节点引用
         */
        void Add(NodeBase &data);

        /**
         * @brief 添加节点到链表头
         * @tparam DType 节点数据类型
         * @param data 节点引用
         */
        template <typename DType>
        FORCE_INLINE void Add(Node<DType> &data)
        {
            this->Add(data.GetSelf());
        }

        /**
         * @brief 从链表中删除节点
         * @param data 节点引用
         * @return 错误码
         */
        ErrorCode Delete(NodeBase &data);

        /**
         * @brief 获取链表大小
         * @return 链表大小
         */
        [[nodiscard]] FORCE_INLINE size_t Size() const
        {
            return size_.load(std::memory_order_relaxed);
        }

        /**
         * @brief 遍历链表
         * @tparam DType 节点数据类型
         * @tparam FType 回调函数类型
         * @tparam Mode 类型检查模式
         * @param func 回调函数
         * @return 错误码
         * @note 回调函数原型为 `ErrorCode func(DType &data)`，返回非`ErrorCode::Ok`会终止遍历
         * @note 如果Mode为`TypeCheckMode::Strict`，则只会回调数据类型完全匹配的节点
         * @note 如果Mode为`TypeCheckMode::Relaxed`，则会回调数据类型大小相同的节点
         * @note 如果Mode为`TypeCheckMode::None`，则不会进行类型检查，可能会导致类型不匹配的节点被回调，使用时请确保类型安全
         */
        template <typename DType, typename FType, Assert::SizeLimitMode Mode = Assert::SizeLimitMode::EQUAL>
            requires std::disjunction_v<std::is_invocable_r<ErrorCode, FType, const DType &>,
                                        std::is_invocable_r<ErrorCode, FType, DType &>>
        ErrorCode ForEach(FType func)
        {
            // 回调函数指针不能为空
            if constexpr (std::is_pointer_v<FType>)
            {
                if (func == nullptr) [[unlikely]]
                    return ErrorCode::INVALID_ARG;
            }

            for (auto pos = this->head_.load(std::memory_order_acquire); pos != nullptr;
                 pos = pos->next_.load(std::memory_order_acquire))
            {
                if constexpr (Mode == Assert::SizeLimitMode::LESS)
                {
                    if (pos->size_ <= sizeof(DType))
                    {
                        if (auto ret = func(*static_cast<Node<DType> &>(*pos));
                            !Check(ret))
                        {
                            return ret;
                        }
                    }
                }
                else if constexpr (Mode == Assert::SizeLimitMode::EQUAL)
                {
                    if (pos->size_ == sizeof(DType))
                    {
                        if (auto ret = func(*static_cast<Node<DType> &>(*pos));
                            !Check(ret))
                        {
                            return ret;
                        }
                    }
                }
                else if constexpr (Mode == Assert::SizeLimitMode::GREAT)
                {
                    if (pos->size_ >= sizeof(DType))
                    {
                        if (auto ret = func(*static_cast<Node<DType> &>(*pos));
                            !Check(ret))
                        {
                            return ret;
                        }
                    }
                }
                else
                {
                    static_assert(false, "Invalid SizeLimitMode");
                }
            }

            return ErrorCode::OK;
        }
    };
}