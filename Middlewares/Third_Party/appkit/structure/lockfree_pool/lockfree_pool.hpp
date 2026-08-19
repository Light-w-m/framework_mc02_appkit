#pragma once

#include <common_type.hpp>
#include <common_assert.hpp>

#include <atomic>
#include <new>
#include <type_traits>

namespace appkit
{
    /**
     * @brief 无锁对象池
     * @tparam DType 对象类型
     */
    template <typename DType>
    class LockFreePool final
    {
        static_assert(std::is_nothrow_copy_constructible_v<DType>, "DType must be nothrow copy constructible");
        static_assert(std::is_standard_layout_v<DType>, "DType must be standard layout");

    public:
        /**
         * @brief 对象槽状态
         */
        enum class SlotState : uint8_t
        {
            EMPTY = 0,
            BUSY,
            READY,
            RECYCLE
        };

        /**
         * @brief 对象槽
         */
        struct alignas(SYSTEM_CACHE_LINE_SIZE) Slot final
        {
            std::atomic<SlotState> state; ///< 状态
            alignas(DType) std::byte storage[sizeof(DType)];

            Slot() noexcept : state(SlotState::EMPTY)
            {
            }

            // 获取对象指针（注意：只有在已构造时才可解引用）
            FORCE_INLINE DType *ptr() noexcept
            {
                return reinterpret_cast<DType *>((storage));
            }

            FORCE_INLINE const DType *ptr() const noexcept
            {
                return reinterpret_cast<const DType *>((storage));
            }

            template <typename... Args>
            FORCE_INLINE void construct(Args &&...args)
            {
                std::construct_at(ptr(), std::forward<Args>(args)...);
            }

            FORCE_INLINE void destroy() noexcept
            {
                std::destroy_at(ptr());
            }
        };

    private:
        const uint32_t capacity_; ///< 对象池容量
        Slot *slots_;             ///< 对象槽数组

        // hint 用于分散 Put/Get 的起始探测索引，减少热点争用
        alignas(SYSTEM_CACHE_LINE_SIZE) std::atomic<uint32_t> put_hint_{0};
        alignas(SYSTEM_CACHE_LINE_SIZE) std::atomic<uint32_t> get_hint_{0};

        // 保证原子操作的无锁性
        static_assert(std::atomic<SlotState>::is_always_lock_free);

    public:
        /**
         * @brief 构造函数
         * @param capacity 对象池容量
         * @note 对象池容量必须大于0
         * @note 如果内存分配失败，会触发断言
         */
        explicit LockFreePool(const uint32_t capacity) : capacity_(capacity)
        {
            // 确保容量是2的幂次方，便于取模运算
            APPKIT_RAISE_IF_NOT(capacity > 0 && (capacity & (capacity - 1)) == 0, "Capacity must be a power of two greater than 0");

            slots_ = new (std::align_val_t{SYSTEM_CACHE_LINE_SIZE}) Slot[capacity];
            APPKIT_RAISE_IF_NOT(slots_ != nullptr, "Memory allocation failed");
            for (uint32_t i = 0; i < capacity_; ++i)
            {
                slots_[i].state.store(SlotState::EMPTY, std::memory_order_release);
            }
        }

        /**
         * @brief 析构函数
         * @note 释放对象槽内存
         */
        ~LockFreePool()
        {
            // 对仍然构造的对象显式析构，避免泄漏/未定义行为
            if (slots_ != nullptr)
            {
                for (uint32_t i = 0; i < capacity_; ++i)
                {
                    auto s = slots_[i].state.load(std::memory_order_acquire);
                    if (s == SlotState::READY || s == SlotState::BUSY || s == SlotState::RECYCLE)
                    {
                        // 只有在非 Empty 的状态下才可能有已构造对象
                        // 尝试析构（若未构造则 UB）
                        slots_[i].destroy();
                    }
                }
                delete[] slots_;
            }
        }

        /**
         * @brief 放入对象
         * @param data 对象引用
         * @return 错误码
         * @note 如果对象池已满，返回ErrorCode::Full
         */
        ErrorCode Put(const DType &data)
        {
            const uint32_t start = put_hint_.fetch_add(1, std::memory_order_relaxed) & (capacity_ - 1);
            for (uint32_t offset = 0; offset < capacity_; ++offset)
            {
                const uint32_t i = (start + offset) & (capacity_ - 1);
                auto expected = slots_[i].state.load(std::memory_order_acquire);
                if (expected == SlotState::EMPTY || expected == SlotState::RECYCLE)
                {
                    if (slots_[i].state.compare_exchange_strong(expected, SlotState::BUSY,
                                                                std::memory_order_acq_rel,
                                                                std::memory_order_acquire))
                    {
                        // 在拿到 Busy 权限后原地构造/赋值对象
                        slots_[i].construct(data);
                        slots_[i].state.store(SlotState::READY, std::memory_order_release);
                        return ErrorCode::OK;
                    }
                }
            }

            return ErrorCode::FULL;
        }

        /**
         * @brief 取出对象
         * @param data 对象引用
         * @return 错误码
         * @note 如果对象池为空，返回ErrorCode::Empty
         */
        ErrorCode Get(DType &data)
        {
            const uint32_t start = get_hint_.fetch_add(1, std::memory_order_relaxed) & (capacity_ - 1);
            for (uint32_t offset = 0; offset < capacity_; ++offset)
            {
                const uint32_t i = (start + offset) & (capacity_ - 1);
                auto expected = slots_[i].state.load(std::memory_order_acquire);
                if (expected == SlotState::READY)
                {
                    if (slots_[i].state.compare_exchange_strong(expected, SlotState::BUSY,
                                                                std::memory_order_acq_rel,
                                                                std::memory_order_acquire))
                    {
                        // 移出对象并析构，标记为 Recycle（可被重用）
                        data = std::move(*slots_[i].ptr());
                        slots_[i].destroy();
                        slots_[i].state.store(SlotState::RECYCLE, std::memory_order_release);
                        return ErrorCode::OK;
                    }
                }
            }

            return ErrorCode::EMPTY;
        }

        /**
         * @brief 获取可用对象槽数量
         * @return 可用对象槽数量
         * @note 可用对象槽数量为状态为Empty或Recycle的对象数量
         */
        size_t Available() const
        {
            size_t available = 0;
            for (uint32_t i = 0; i < capacity_; ++i)
            {
                if (auto expected = slots_[i].state.load(std::memory_order_relaxed);
                    expected == SlotState::EMPTY || expected == SlotState::RECYCLE)
                {
                    ++available;
                }
            }

            return available;
        }

        /**
         * @brief 获取对象池可用对象数量
         * @return 对象池可用对象数量
         * @note 对象池可用对象数量为状态为Ready的对象数量
         */
        size_t Size() const
        {
            size_t size = 0;
            for (uint32_t i = 0; i < capacity_; ++i)
            {
                if (auto expected = slots_[i].state.load(std::memory_order_relaxed);
                    expected == SlotState::READY)
                {
                    ++size;
                }
            }

            return size;
        }
    };
} // namespace appkit
