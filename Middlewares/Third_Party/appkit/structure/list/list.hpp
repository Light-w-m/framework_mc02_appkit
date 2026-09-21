#pragma once

#include <memory>
#include <type_traits>

#include <common_type.hpp>
#include <common_assert.hpp>
#include <osal_mutex.hpp>

namespace appkit
{
    /**
     * @brief 链表实现，用于存储和管理数据节点
     * 
     */
    class List final
    {
    public:
        /**
         * @brief 链表基础节点，所有节点都继承自该类
         */
        class BaseNode
        {
        public:
            BaseNode(size_t size);

            ~BaseNode();

            BaseNode *next_ = nullptr; ///< 下一个节点指针
            size_t size_;    ///< 节点数据大小
        };

        /**
         * @brief 链表节点，用于存储具体类型的数据
         */
        template <typename DType>
        class Node : public BaseNode
        {
        public:
            Node() : BaseNode(sizeof(DType)) {}

            /**
             * @brief 使用指定数据初始化节点
             * 
             * @param data 
             */
            explicit Node(const DType& data) : BaseNode(sizeof(DType)), data_(data) {}

            /**
             * @brief 使用参数包初始化节点
             * 
             * @tparam Args 
             * @param args 
             */
            template <typename... Args>
            explicit Node(Args&&... args) 
            : BaseNode(sizeof(DType)), data_(std::forward<Args>(args)...) {}

            /**
             * @brief 赋值运算符重载，允许直接对节点赋值
             * 
             * @param data 
             * @return Node& 
             */
            Node& operator=(const DType& data)
            {
                data_ = data;
                return *this;
            }

            /**
             * @brief 操作符重载，提供数据访问接口
             * 
             * @return DType* 
             */
            DType* operator->() noexcept { return &data_; }
            const DType* operator->() const noexcept { return &data_; }
            const DType& operator*() const noexcept { return data_; }
            DType& operator*() noexcept { return data_; }
            operator DType&() noexcept { return data_; }

            DType data_; ///< 节点存储的数据

        };

        /**
         * @brief 构造函数，初始化链表
         */
        List() noexcept;

        ~List() noexcept;

        /**
         * @brief 向链表中添加节点
         * 
         * @param node 
         */
        void Add(BaseNode* node);

        /**
         * @brief 获取链表中的节点数量
         * 
         * @param node 
         */
        uint32_t Size() noexcept;

        /**
         * @brief 删除链表中的节点
         * 
         * @param node 
         * @return ErrorCode 
         */
        ErrorCode Delete(BaseNode* node) noexcept;

        /**
         * @brief 遍历链表，并对符合类型检查的节点应用回调函数
         *
         * @tparam Data 节点存储的数据类型
         * @tparam Func 回调函数类型
         * @tparam LimitMode 类型检查模式
         * @param func 回调函数，原型为 `ErrorCode func(Node<Data> &node)`
         * @return ErrorCode 遍历结果，回调返回非 `ErrorCode::OK` 时立即终止并透传该错误码
         * @note LimitMode 为 LESS 时仅回调 size_ <= sizeof(Data) 的节点
         * @note LimitMode 为 EQUAL 时仅回调 size_ == sizeof(Data) 的节点
         * @note LimitMode 为 GREAT 时仅回调 size_ >= sizeof(Data) 的节点
         * @note 遍历期间持有互斥锁，回调内不得对同一链表调用 Add/Delete，否则死锁
         */
        template <typename Data, typename Func, Assert::SizeLimitMode LimitMode = Assert::SizeLimitMode::GREAT>
            requires std::disjunction_v<std::is_invocable_r<ErrorCode, Func, const Node<Data> &>,
                                        std::is_invocable_r<ErrorCode, Func, Node<Data> &>>
        ErrorCode ForEach(Func func)
        {
            // 回调函数指针不能为空
            if constexpr (std::is_pointer_v<Func>)
            {
                if (func == nullptr) [[unlikely]]
                {
                    return ErrorCode::INVALID_ARG;
                }
            }

            [[maybe_unused]] osal::LockGuard lock(mutex_); ///< 作用域锁，退出时自动解锁

            for (auto pos = head_.next_; pos != &head_; pos = pos->next_)
            {
                if constexpr (Assert::SizeLimitMode::LESS == LimitMode)
                {
                    if (pos->size_ > sizeof(Data))
                    {
                        continue;
                    }
                }
                else if constexpr (Assert::SizeLimitMode::EQUAL == LimitMode)
                {
                    if (pos->size_ != sizeof(Data))
                    {
                        continue;
                    }
                }
                else if constexpr (Assert::SizeLimitMode::GREAT == LimitMode)
                {
                    if (pos->size_ < sizeof(Data))
                    {
                        continue;
                    }
                }
                else
                {
                    static_assert(false, "Invalid SizeLimitMode");
                }

                if (auto ret = func(static_cast<Node<Data> &>(*pos)); !Check(ret))
                {
                    return ret;
                }
            }

            return ErrorCode::OK;
        }

    private:
        BaseNode head_; ///< 链表头节点
        osal::Mutex mutex_; ///< 互斥锁，用于保护链表操作
    };
    
    
} // namespace appkit
