/**
 * @file osal_taskpool.hpp
 * @brief OSAL固定容量任务池
 */
#pragma once

#include <osal_queue.hpp>
#include <osal_semaphore.hpp>
#include <osal_thread.hpp>

#include <common_assert.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace appkit::osal
{
    /**
     * @brief 固定容量任务池，用于并发处理一次性后台任务
     * @note 任务槽位在构造任务池时一次性分配，投递任务时不会进行堆内存分配
     * @note 普通 Submit 接口只能在线程上下文调用；中断/回调上下文使用 SubmitFromCallback
     * @note Stop 会先执行完已经成功投递的任务，再停止所有工作线程
     */
    class TaskPool final
    {
        DECL_COPY_DISABLE(TaskPool)
        DECL_MOVE_DISABLE(TaskPool)

    public:
        static constexpr std::size_t TASK_STORAGE_SIZE = 64; ///< 单个任务可使用的最大内联存储字节数

    private:
        /**
         * @brief 任务池运行状态
         */
        enum class State : uint8_t
        {
            CREATED = 0,
            STARTING,
            RUNNING,
            STOPPING,
            STOPPED,
        };

        /**
         * @brief 任务槽位状态
         */
        enum class SlotState : uint8_t
        {
            FREE = 0,
            CONSTRUCTING,
            QUEUED,
            RUNNING,
        };

        static_assert(std::atomic<State>::is_always_lock_free,
                      "TaskPool state atomics must be lock-free");
        static_assert(std::atomic<SlotState>::is_always_lock_free,
                      "TaskPool slot atomics must be lock-free");
        static_assert(std::atomic<std::size_t>::is_always_lock_free,
                      "TaskPool counters must be lock-free");

        /**
         * @brief 任务槽位，保存任务状态和内联存储
         */
        struct TaskSlot final
        {
            std::atomic<SlotState> state{SlotState::FREE}; ///< 任务槽位状态
            void (*invoke)(void *) noexcept{};             ///< 任务执行函数
            void (*destroy)(void *) noexcept{};            ///< 任务析构函数
            alignas(std::max_align_t) std::byte storage[TASK_STORAGE_SIZE]{}; ///< 任务对象存储空间
        };

        /**
         * @brief 任务对象模型，保存可调用对象及其参数
         * @tparam FType 可调用对象类型
         * @tparam Args 任务参数类型
         */
        template <typename FType, typename... Args>
        struct TaskModel final
        {
            [[no_unique_address]] FType func; ///< 可调用对象
            std::tuple<Args...> args;          ///< 调用参数

            template <typename Func, typename... TaskArgs>
            explicit TaskModel(Func &&task_func, TaskArgs &&...task_args) noexcept(
                std::is_nothrow_constructible_v<FType, Func &&> &&
                std::is_nothrow_constructible_v<std::tuple<Args...>, TaskArgs &&...>)
                : func(std::forward<Func>(task_func)),
                  args(std::forward<TaskArgs>(task_args)...)
            {
            }

            /**
             * @brief 执行任务
             */
            void Run() noexcept
            {
                std::apply(
                    [this](auto &...task_args) noexcept
                    {
                        std::invoke(std::move(func), std::move(task_args)...);
                    },
                    args);
            }
        };

        const std::size_t worker_count_;  ///< 工作线程数量
        const std::size_t task_capacity_; ///< 任务槽位数量
        Queue<TaskSlot *> task_queue_;    ///< 待执行任务队列
        Semaphore worker_exit_sem_{};     ///< 工作线程退出信号量
        Thread *workers_{};               ///< 工作线程数组
        TaskSlot *slots_{};               ///< 任务槽位数组

        std::atomic<State> state_{State::CREATED};       ///< 任务池运行状态
        std::atomic<std::size_t> slot_hint_{};            ///< 下次查找任务槽位的起始位置
        std::atomic<std::size_t> active_submitters_{};   ///< 当前正在投递任务的线程数
        std::atomic<std::size_t> outstanding_tasks_{};   ///< 已投递但尚未完成的任务数
        std::size_t created_workers_{};                  ///< 已创建的工作线程数量

        /**
         * @brief 校验数量参数
         * @param value 待校验的数量
         * @param message 参数无效时的错误信息
         * @return 校验后的数量
         */
        static std::size_t ValidateCount_(std::size_t value, const char *message);

        /**
         * @brief 工作线程入口函数
         * @param self 任务池对象指针
         */
        static void WorkerEntry_(TaskPool *self);

        /**
         * @brief 执行工作线程循环
         */
        void WorkerLoop_();

        /**
         * @brief 停止指定数量的工作线程
         * @param count 待停止的工作线程数量
         */
        void StopWorkers_(std::size_t count);

        /**
         * @brief 判断当前线程是否为本任务池的工作线程
         * @return 当前线程属于本任务池时返回 true
         */
        [[nodiscard]] bool IsWorkerThread_() const;

        /**
         * @brief 开始投递任务
         * @return 允许投递时返回 true
         */
        [[nodiscard]] bool BeginSubmit_();

        /**
         * @brief 结束投递任务
         */
        void EndSubmit_();

        /**
         * @brief 获取空闲任务槽位
         * @param timeout 等待空闲任务槽位的时间
         * @param error 获取失败时写入的错误码
         * @return 获取到的任务槽位，失败时返回 nullptr
         */
        [[nodiscard]] TaskSlot *AcquireSlot_(const Duration &timeout, ErrorCode &error);

        /**
         * @brief 释放任务槽位
         * @param slot 待释放的任务槽位
         */
        void ReleaseSlot_(TaskSlot &slot) noexcept;

        template <typename FType, typename... Args>
        ErrorCode EmplaceTask_(const Duration &timeout, FType &&func, Args &&...args)
        {
            using Model = TaskModel<std::decay_t<FType>, std::decay_t<Args>...>;

            static_assert(sizeof(Model) <= TASK_STORAGE_SIZE,
                          "Task is too large for TaskPool::TASK_STORAGE_SIZE");
            static_assert(alignof(Model) <= alignof(std::max_align_t),
                          "Task alignment exceeds TaskPool slot alignment");
            static_assert(std::is_nothrow_constructible_v<Model, FType &&, Args &&...>,
                          "Task callable and arguments must be nothrow constructible");
            static_assert(std::is_nothrow_destructible_v<Model>,
                          "Task callable and arguments must be nothrow destructible");

            if constexpr (std::is_pointer_v<std::decay_t<FType>>)
            {
                if (func == nullptr)
                {
                    return ErrorCode::INVALID_ARG;
                }
            }

            if (!BeginSubmit_())
            {
                return ErrorCode::BUSY;
            }

            ErrorCode acquire_error{};
            auto *slot = AcquireSlot_(timeout, acquire_error);
            if (slot == nullptr)
            {
                EndSubmit_();
                return acquire_error;
            }

            auto *model = std::construct_at(
                reinterpret_cast<Model *>(slot->storage),
                std::forward<FType>(func), std::forward<Args>(args)...);

            slot->invoke = +[](void *storage) noexcept
            {
                static_cast<Model *>(storage)->Run();
            };
            slot->destroy = +[](void *storage) noexcept
            {
                std::destroy_at(static_cast<Model *>(storage));
            };
            slot->state.store(SlotState::QUEUED, std::memory_order_release);

            outstanding_tasks_.fetch_add(1, std::memory_order_acq_rel);
            const auto ret = task_queue_.Push(slot, timeout);
            if (ret != ErrorCode::OK)
            {
                outstanding_tasks_.fetch_sub(1, std::memory_order_acq_rel);
                slot->destroy(model);
                ReleaseSlot_(*slot);
            }

            EndSubmit_();
            return ret;
        }

    public:
        /**
         * @brief 构造任务池
         * @param worker_count 工作线程数量，必须大于0
         * @param task_capacity 最大并发任务数量，必须大于0
         */
        explicit TaskPool(std::size_t worker_count, std::size_t task_capacity);

        /**
         * @brief 析构任务池
         * @note 若任务池仍在运行，会阻塞等待所有已投递任务执行完毕
         */
        ~TaskPool();

        /**
         * @brief 启动工作线程
         * @param name 工作线程名称
         * @param stack_depth 每个工作线程的栈深度（字节）
         * @param priority 工作线程优先级
         */
        [[nodiscard]] ErrorCode Start(const char *name, std::size_t stack_depth,
                                      Thread::Priority priority = Thread::Priority::NORMAL);

        /**
         * @brief 停止任务池
         * @note 先拒绝新任务，再执行完已成功投递的任务，最后停止工作线程
         * @note 不允许在本任务池的工作线程或中断上下文中调用
         */
        [[nodiscard]] ErrorCode Stop();

        /**
         * @brief 非阻塞投递任务
         * @return OK、FULL、BUSY、NOT_SUPPORTED或底层队列错误码
         */
        template <typename FType, typename... Args>
            requires std::is_invocable_r_v<void, std::decay_t<FType>, std::decay_t<Args>...>
        [[nodiscard]] ErrorCode Submit(FType &&func, Args &&...args)
        {
            if (CheckInIsr())
            {
                return ErrorCode::NOT_SUPPORTED;
            }
            return EmplaceTask_(Duration::Zero(), std::forward<FType>(func), std::forward<Args>(args)...);
        }

        /**
         * @brief 带超时投递任务
         * @param timeout 等待空闲任务槽位的时间
         */
        template <typename FType, typename... Args>
            requires std::is_invocable_r_v<void, std::decay_t<FType>, std::decay_t<Args>...>
        [[nodiscard]] ErrorCode SubmitFor(const Duration &timeout, FType &&func, Args &&...args)
        {
            if (CheckInIsr())
            {
                return ErrorCode::NOT_SUPPORTED;
            }
            return EmplaceTask_(timeout, std::forward<FType>(func), std::forward<Args>(args)...);
        }

        /**
         * @brief 从中断或驱动回调上下文非阻塞投递轻量任务
         * @note 任务对象必须可平凡复制，避免在中断上下文运行复杂构造/析构逻辑
         */
        template <typename FType, typename... Args>
            requires std::is_invocable_r_v<void, std::decay_t<FType>, std::decay_t<Args>...>
        [[nodiscard]] ErrorCode SubmitFromCallback(FType &&func, Args &&...args)
        {
            static_assert(std::is_trivially_copyable_v<std::decay_t<FType>> &&
                              (std::is_trivially_copyable_v<std::decay_t<Args>> && ...),
                          "Callback tasks must contain only trivially copyable state");
            return EmplaceTask_(Duration::Zero(), std::forward<FType>(func), std::forward<Args>(args)...);
        }

        /**
         * @brief 查询任务池是否正在运行
         * @return 任务池正在运行时返回 true
         */
        [[nodiscard]] bool IsRunning() const;

        /**
         * @brief 获取工作线程数量
         * @return 工作线程数量
         */
        [[nodiscard]] std::size_t WorkerCount() const;

        /**
         * @brief 获取任务池容量
         * @return 最大任务数量
         */
        [[nodiscard]] std::size_t Capacity() const;

        /**
         * @brief 获取待执行任务数量
         * @return 待执行任务数量
         */
        [[nodiscard]] std::size_t Pending() const;

        /**
         * @brief 获取可用任务槽位数量
         * @return 可用任务槽位数量
         */
        [[nodiscard]] std::size_t Available() const;
    };
} // namespace appkit::osal
