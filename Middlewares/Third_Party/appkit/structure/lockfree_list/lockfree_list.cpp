/**
 * @file lockfree_list.cpp
 * @brief 无锁链表实现文件
 * @author dusk
 */
#include "lockfree_list.hpp"

namespace appkit
{
    LockFreeList::NodeBase::~NodeBase()
    {
        APPKIT_RAISE_IF_NOT(next_.load(std::memory_order_acquire) == nullptr, "Node is still in the list");
    }

    LockFreeList::~LockFreeList()
    {
        size_.store(0, std::memory_order_release);

        for (auto pos = head_.exchange(nullptr); pos != nullptr; pos = pos->next_.exchange(nullptr))
        {
        }
    }

    void LockFreeList::Add(NodeBase &data)
    {
        NodeBase *current_head = nullptr;
        do
        {
            current_head = head_.load(std::memory_order_acquire);
            data.next_.store(current_head, std::memory_order_release);
        } while (!head_.compare_exchange_weak(current_head, &data,
                                              std::memory_order_release,
                                              std::memory_order_acquire));

        size_.fetch_add(1, std::memory_order_release);
    }

    ErrorCode LockFreeList::Delete(NodeBase &data)
    {
        NodeBase *data_ptr = &data;
        for (auto current_parent = &this->head_; current_parent->load(std::memory_order_acquire) != nullptr;
             current_parent = &current_parent->load(std::memory_order_acquire)->next_)
        {
            if (current_parent->compare_exchange_weak(data_ptr, data.next_.load(std::memory_order_acquire),
                                                      std::memory_order_seq_cst))
            {
                data.next_.store(nullptr, std::memory_order_release);

                size_.fetch_sub(1, std::memory_order_release);
                return ErrorCode::OK;
            }
        }

        return ErrorCode::NO_FOUND;
    }
}