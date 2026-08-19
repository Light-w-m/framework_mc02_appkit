/**
 * @file osal_timer.h
 * @brief OSAL定时器头文件
 * @author dusk
 */
#pragma once

#include <osal_thread.hpp>
#include <common_type.hpp>

#include <lockfree_list.hpp>

namespace appkit::osal
{
    class Timer final
    {
    private:
        static Thread timer_thread_; ///< 定时器线程
        static LockFreeList list_;   ///< 定时器任务链表

        static Duration interval_; ///< 定时器刷新间隔

        /**
         * @brief 定时器刷新函数
         */
        [[noreturn]] static void ReflashFunc();

    public:
        static uint32_t stack_depth;      ///< 定时器线程栈深度
        static Thread::Priority priority; ///< 定时器线程优先级

        /**
         * @brief 定时器任务信息
         */
        class TaskInfo final
        {
            uint32_t count_{}; ///< 计数器

            friend class Timer;

            /**
             * @brief 运行任务函数
             */
            FORCE_INLINE void Run() const
            {
                if (nullptr != func)
                {
                    func(handle);
                }
            }

        public:
            void (*func)(void *handle){}; ///< 任务函数
            void *handle{};               ///< 任务句柄
            uint32_t period{};            ///< 任务周期，单位毫秒
            bool enabled{};               ///< 任务使能标志
        };

        using TimerHandle = LockFreeList::Node<TaskInfo> *; ///< 定时器任务句柄类型

        /**
         * @brief 启动定时器服务
         * @return 错误码
         * @note 如果定时器线程已启动，则返回ErrorCode::Busy
         */
        static ErrorCode Start();

        /**
         * @brief 添加定时器任务
         * @param handle 定时器任务句柄
         */
        FORCE_INLINE static void Add(std::add_const_t<TimerHandle> handle)
        {
            list_.Add(*handle);
        }

        /**
         * @brief 刷新定时器任务
         * @note 该函数会检查所有定时器任务，并执行到期的任务
         */
        static void Reflash();

        /**
         * @brief 创建定时器任务
         * @tparam FType 任务函数类型
         * @tparam Args 任务参数类型
         * @param func 任务函数
         * @param period 任务周期，单位毫秒，必须大于0
         * @param arg 任务参数
         * @return 定时器任务句柄
         * @note 任务函数原型为 `void func(Args arg)` 或 `void func(const Args &arg)`
         * @note 任务周期必须大于0
         * @note 如果内存分配失败，会触发断言
         */
        template <typename FType, typename Arg>
            requires std::is_invocable_r_v<void, FType, Arg>
        static TimerHandle Create(FType func, Arg arg, uint32_t period)
        {
            if (period == 0)
            {
                APPKIT_RAISE("Timer period must be greater than 0");
                return nullptr;
            }

            if constexpr (std::is_pointer_v<FType>)
            {
                if (nullptr == func)
                {
                    APPKIT_RAISE("Timer function pointer must not be null");
                    return nullptr;
                }
            }

            struct Block
            {
                LockFreeList::Node<TaskInfo> info;
                FType func;
                Arg arg;
            };

            auto *block = new Block{{}, func, arg};

            (*block->info).func = [](void *handle)
            {
                auto &[_, func, arg] = *static_cast<Block *>(handle);
                func(arg);
            };
            (*block->info).handle = block;
            (*block->info).period = period;
            (*block->info).enabled = true;

            return std::addressof(block->info);
        }

        /**
         * @brief 创建无参数定时器任务
         * @tparam FType 任务函数类型
         * @param func 任务函数
         * @param period 任务周期，单位毫秒，必须大于0
         * @return 定时器任务句柄
         * @note 任务函数原型为 `void func(Args arg)` 或 `void func(const Args &arg)`
         * @note 任务周期必须大于0
         * @note 如果内存分配失败，会触发断言
         */
        template <typename FType>
            requires std::is_invocable_r_v<void, FType>
        static TimerHandle Create(FType func, uint32_t period)
        {
            if (period == 0)
            {
                APPKIT_RAISE("Timer period must be greater than 0");
                return nullptr;
            }

            if constexpr (std::is_pointer_v<FType>)
            {
                if (nullptr == func)
                {
                    APPKIT_RAISE("Timer function pointer must not be null");
                    return nullptr;
                }
            }

            struct Block
            {
                LockFreeList::Node<TaskInfo> info;
                FType func;
            };

            auto *block = new Block{{}, func};

            (*block->info).func = [](void *handle)
            {
                auto &[_, func] = *static_cast<Block *>(handle);
                func();
            };
            (*block->info).handle = block;
            (*block->info).period = period;
            (*block->info).enabled = true;

            return std::addressof(block->info);
        }
    };
} // namespace appkit::osal