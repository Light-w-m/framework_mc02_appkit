#pragma once

#include <atomic>

#include <common_type.hpp>
#include <common_assert.hpp>

namespace appkit
{
    /**
     * @brief 无锁队列
     * @tparam DType 队列元素类型
     */
    template <typename DType>
        requires std::is_nothrow_move_constructible_v<DType> && std::is_nothrow_move_assignable_v<DType>
    class alignas(SYSTEM_CACHE_LINE_SIZE) LockFreeQueue
    {
    private:
        // 保证原子操作的无锁性
        static_assert(std::atomic<size_t>::is_always_lock_free);

        const size_t capacity_{0};                                    ///< 队列容量（实际可用长度为capacity_，内部会多分配一个元素用于区分队列满和队列空）
        DType *queue_{nullptr};                                       ///< 队列内存
        alignas(SYSTEM_CACHE_LINE_SIZE) std::atomic<size_t> head_{0}; ///< 头索引
        alignas(SYSTEM_CACHE_LINE_SIZE) std::atomic<size_t> tail_{0}; ///< 尾索引

        /**
         * @brief 获取下一个索引
         * @param index 当前索引
         * @param increment 增量，默认为1
         * @return 下一个索引
         * @note increment必须小于等于capacity_
         * @note 如果increment大于capacity_，会触发断言
         * @note 如果队列长度为N，则索引范围为0~N（包含N），N表示队列空
         * @note 如果索引加上增量超过N，则从0开始计数
         */
        [[nodiscard]] FORCE_INLINE size_t Increment(const size_t index, const size_t increment = 1) const
        {
            return (index + increment) % (capacity_ + 1);
        }

    public:
        LockFreeQueue() = default;

        /**
         * @brief 构造函数
         * @param length 队列长度
         * @note 队列长度必须大于0
         * @note 队列实际可用长度为length，内部会多分配一个元素用于区分队列满和队列空
         * @note 队列内存按系统缓存行对齐分配
         * @note 如果内存分配失败，会触发断言
         */
        explicit LockFreeQueue(const size_t length) : capacity_(length), queue_(new (std::align_val_t{SYSTEM_CACHE_LINE_SIZE}) DType[length + 1])
        {
            APPKIT_RAISE_IF_NOT(capacity_ > 0, "Queue length must be greater than 0");
            APPKIT_RAISE_IF_NOT(queue_ != nullptr, "Failed to allocate memory for queue");
        }

        /**
         * @brief 析构函数
         * @note 释放队列内存
         * @note 如果队列内存为空指针，则不进行释放
         */
        ~LockFreeQueue() noexcept
        {
            delete[] queue_;
        }

        /**
         * @brief 入队
         * @param data 数据引用
         * @return 错误码
         * @note 如果队列已满，返回ErrorCode::Full
         */
        ErrorCode Push(const DType &data)
        {
            size_t current_tail{0}, next_tail{0};
            do
            {
                current_tail = tail_.load(std::memory_order_acquire);
                next_tail = Increment(current_tail);

                if (head_.load(std::memory_order_acquire) == next_tail)
                {
                    return ErrorCode::FULL;
                }
            } while (!tail_.compare_exchange_weak(current_tail, next_tail, std::memory_order_acquire,
                                                  std::memory_order_relaxed));

            std::construct_at(&queue_[current_tail], data);
            return ErrorCode::OK;
        }

        /**
         * @brief 入队（移动语义）
         * @param data 数据右值引用
         * @return 错误码
         * @note 如果队列已满，返回ErrorCode::Full
         */
        ErrorCode PushBatch(const DType *data, const size_t length)
        {
            APPKIT_RAISE_IF_NOT(data != nullptr, "Data pointer must not be null");

            size_t current_tail{0}, next_tail{0};
            do
            {
                if (Available() < length)
                {
                    return ErrorCode::OUT_OF_RANGE;
                }

                current_tail = tail_.load(std::memory_order_acquire);
                next_tail = Increment(current_tail, length);

                if (head_.load(std::memory_order_acquire) == next_tail)
                {
                    return ErrorCode::FULL;
                }
            } while (!tail_.compare_exchange_weak(current_tail, next_tail, std::memory_order_acquire,
                                                  std::memory_order_relaxed));

            for (size_t i = 0; i < length; ++i)
            {
                std::construct_at(&queue_[Increment(current_tail, i)], data[i]);
            }

            return ErrorCode::OK;
        }

        /**
         * @brief 出队
         * @param data 数据引用
         * @return 错误码
         * @note 如果队列为空，返回ErrorCode::Empty
         */
        ErrorCode Pop(DType &data)
        {
            size_t current_head{0}, next_head{0};
            do
            {
                current_head = head_.load(std::memory_order_acquire);

                if (current_head == tail_.load(std::memory_order_acquire))
                {
                    return ErrorCode::EMPTY;
                }

                next_head = Increment(current_head);
                data = std::move(queue_[current_head]);
            } while (!head_.compare_exchange_weak(current_head, next_head, std::memory_order_acquire,
                                                  std::memory_order_relaxed));

            return ErrorCode::OK;
        }

        /**
         * @brief 出队
         * @return 错误码
         * @note 如果队列为空，返回ErrorCode::Empty
         */
        ErrorCode Pop()
        {
            size_t current_head{0}, next_head{0};
            do
            {
                current_head = head_.load(std::memory_order_acquire);

                if (current_head == tail_.load(std::memory_order_acquire))
                {
                    return ErrorCode::EMPTY;
                }

                next_head = Increment(current_head);
            } while (!head_.compare_exchange_weak(current_head, next_head, std::memory_order_acquire,
                                                  std::memory_order_relaxed));

            return ErrorCode::OK;
        }

        /**
         * @brief 出队（批量）
         * @param length 出队长度
         * @return 错误码
         * @note 如果队列可用数据小于length，返回ErrorCode::OutOfRange
         * @note 如果队列为空，返回ErrorCode::Empty
         */
        ErrorCode PopBatch(const size_t length)
        {
            size_t current_head{0}, next_head{0};
            do
            {
                if (Size() < length)
                {
                    return ErrorCode::OUT_OF_RANGE;
                }

                current_head = head_.load(std::memory_order_acquire);

                if (current_head == tail_.load(std::memory_order_acquire))
                {
                    return ErrorCode::EMPTY;
                }

                next_head = Increment(current_head, length);
            } while (!head_.compare_exchange_weak(current_head, next_head, std::memory_order_acquire,
                                                  std::memory_order_relaxed));

            return ErrorCode::OK;
        }

        /**
         * @brief 出队（批量）
         * @param data 数据指针
         * @param length 出队长度
         * @return 错误码
         * @note 如果data为nullptr，触发断言
         * @note 如果队列可用数据小于length，返回ErrorCode::OutOfRange
         * @note 如果队列为空，返回ErrorCode::Empty
         */
        ErrorCode PopBatch(DType *data, const size_t length)
        {
            APPKIT_RAISE_IF_NOT(data != nullptr, "Data pointer must not be null");

            size_t current_head{0}, next_head{0};
            do
            {
                if (Size() < length)
                {
                    return ErrorCode::OUT_OF_RANGE;
                }

                current_head = head_.load(std::memory_order_acquire);

                if (current_head == tail_.load(std::memory_order_acquire))
                {
                    return ErrorCode::EMPTY;
                }

                for (size_t i = 0; i < length; ++i)
                {
                    data[i] = queue_[Increment(current_head, i)];
                }

                next_head = Increment(current_head, length);
            } while (!head_.compare_exchange_weak(current_head, next_head, std::memory_order_acquire,
                                                  std::memory_order_relaxed));

            return ErrorCode::OK;
        }

        /**
         * @brief 查看队首元素
         * @param data 数据引用
         * @return 错误码
         * @note 如果队列为空，返回ErrorCode::Empty
         */
        ErrorCode Peek(DType &data)
        {
            size_t current_head{0};
            do
            {
                current_head = head_.load(std::memory_order_acquire);

                if (current_head == tail_.load(std::memory_order_acquire))
                {
                    return ErrorCode::EMPTY;
                }

                data = queue_[current_head];
            } while (current_head != head_.load(std::memory_order_acquire));

            return ErrorCode::OK;
        }

        /**
         * @brief 查看队首元素（批量）
         * @param data 数据指针
         * @param length 查看长度
         * @return 错误码
         * @note 如果data为nullptr，触发断言
         * @note 如果队列可用数据小于length，返回ErrorCode::OutOfRange
         * @note 如果队列为空，返回ErrorCode::Empty
         */
        ErrorCode PeekBatch(DType *data, const size_t length)
        {
            APPKIT_RAISE_IF_NOT(data != nullptr, "Data pointer must not be null");

            size_t current_head{0};
            do
            {
                current_head = head_.load(std::memory_order_acquire);

                if (current_head == tail_.load(std::memory_order_acquire))
                {
                    return ErrorCode::EMPTY;
                }

                if (Size() < length)
                {
                    return ErrorCode::OUT_OF_RANGE;
                }

                for (size_t i = 0; i < length; ++i)
                {
                    data[i] = queue_[Increment(current_head, i)];
                }
            } while (current_head != head_.load(std::memory_order_acquire));

            return ErrorCode::OK;
        }

        /**
         * @brief 重置队列
         */
        void Reset()
        {
            head_.store(0, std::memory_order_release);
            tail_.store(0, std::memory_order_release);
        }

        /**
         * @brief 获取队列容量
         * @return 队列容量
         * @note 队列容量为构造函数传入的length值
         */
        [[nodiscard]] FORCE_INLINE size_t Capacity() const
        {
            return capacity_;
        }

        /**
         * @brief 获取队列当前元素数量
         * @return 队列当前元素数量
         * @note 如果队列为空，返回0
         * @note 如果队列为满，返回capacity()
         */
        [[nodiscard]] size_t Size() const
        {
            const size_t current_head = head_.load(std::memory_order_acquire);
            const size_t current_tail = tail_.load(std::memory_order_acquire);

            return (current_tail >= current_head) ? (current_tail - current_head)
                                                  : (capacity_ + current_tail - current_head);
        }

        /**
         * @brief 获取队列可用空间
         * @return 队列可用空间
         * @note 如果队列为空，返回capacity()
         * @note 如果队列为满，返回0
         */
        [[nodiscard]] size_t Available() const
        {
            return Capacity() - Size();
        }
    };
}