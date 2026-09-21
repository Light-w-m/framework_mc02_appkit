/**
 * @file list.cpp
 * @brief 链表实现文件
 */
#include <list.hpp>

namespace appkit
{
    List::BaseNode::BaseNode(size_t size) : size_(size)
    {
    }

    List::BaseNode::~BaseNode()
    {
        APPKIT_RAISE_IF_NOT(next_ == nullptr, "Node is still in the list");
    }

    List::List() noexcept : head_(0)
    {
        head_.next_ = &head_;
    }

    List::~List() noexcept
    {
        // 摘除所有节点，使各节点的析构断言得以通过
        for (auto pos = head_.next_; pos != &head_;)
        {
            auto next = pos->next_;
            pos->next_ = nullptr;
            pos = next;
        }

        head_.next_ = nullptr;
    }

    void List::Add(BaseNode *node)
    {
        [[maybe_unused]] osal::LockGuard lock(mutex_);

        node->next_ = head_.next_;
        head_.next_ = node;
    }

    uint32_t List::Size() noexcept
    {
        uint32_t size = 0;

        [[maybe_unused]] osal::LockGuard lock(mutex_);

        for (auto pos = head_.next_; pos != &head_; pos = pos->next_)
        {
            ++size;
        }

        return size;
    }

    ErrorCode List::Delete(BaseNode *node) noexcept
    {
        [[maybe_unused]] osal::LockGuard lock(mutex_);

        for (auto pos = &head_; pos->next_ != &head_; pos = pos->next_)
        {
            if (pos->next_ == node)
            {
                pos->next_ = node->next_;
                node->next_ = nullptr;
                return ErrorCode::OK;
            }
        }

        return ErrorCode::NO_FOUND;
    }
}
