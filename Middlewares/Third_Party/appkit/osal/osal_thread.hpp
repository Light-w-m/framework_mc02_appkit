/**
 * @file osal_thread.hpp
 * @brief OSAL线程头文件
 * @author dusk
 */
#pragma once

#include <common_time.hpp>
#include <common_assert.hpp>

#include <osal_def.hpp>

namespace appkit::osal
{
    class Thread final
    {
        DECL_COPY_DISABLE(Thread)

    public:
        /**
         * @brief 线程优先级
         * @enum Priority
         */
        enum class Priority : uint8_t
        {
            REALTIME = 0,
            HIGH,
            NORMAL,
            LOW,
            IDLE,
        };

    private:
        ThreadId handle_; ///< 任务句柄

        /**
         * @brief
         * @param thread_base ThreadBase指针
         * @param func 任务函数
         * @param name 任务名称
         * @param stack_depth 任务栈深度
         * @param priority 任务优先级
         */
        ErrorCode Create_(void *thread_base, void (*func)(void *), const char *name, size_t stack_depth,
                          Priority priority);

        /**
         * @brief 线程退出函数
         */
        static void Exit_();

    public:
        /**
         * @brief 空构造函数
         */
        constexpr Thread() = default;

        /**
         * @brief 构造函数
         * @param handle 底层任务句柄
         */
        explicit Thread(const ThreadId handle) : handle_(handle)
        {
        }

        /**
         * @brief 创建任务函数
         * @tparam AType 任务参数类型
         * @param arg 任务参数
         * @param func 任务函数
         * @param name 任务名
         * @param stack_depth 任务栈深度
         * @param priority 任务优先级
         */
        template <typename Args, typename FType>
            requires(std::is_trivially_copyable_v<Args> && std::is_invocable_r_v<void, FType, Args>)
        ErrorCode Create(FType func, Args arg, const char *name, size_t stack_depth,
                         Priority priority = Priority::NORMAL)
        {
            APPKIT_RAISE_IF_NOT(!IsValid(), "Thread has already been created"); // 确保未创建过线程
            if constexpr (std::is_pointer_v<FType>)
            {
                if (nullptr == func)
                {
                    APPKIT_RAISE("Thread function pointer must not be null"); // 确保函数指针有效
                    return ErrorCode::INVALID_ARG;
                }
            }

            struct ThreadBase final
            {
                FType func_;
                Args arg_;
            };

            auto base = new ThreadBase(func, arg);
            static auto base_func = +[](void *arg) -> void
            {
                auto *self = static_cast<ThreadBase *>(arg);
                self->func_(self->arg_);

                delete self;
                Exit_();
            };

            auto ret = this->Create_(base, base_func, name, stack_depth, priority);
            if (ret != ErrorCode::OK)
            {
                delete base;
            }

            return ret;
        }

        /**
         * @brief 创建无参数任务函数
         * @tparam FType 任务函数类型
         * @param func 任务函数
         * @param name 任务名
         * @param stack_depth 任务栈深度
         * @param priority 任务优先级
         */
        template <typename FType>
            requires std::is_invocable_r_v<void, FType>
        ErrorCode Create(FType func, const char *name, size_t stack_depth,
                         Priority priority = Priority::NORMAL)
        {
            APPKIT_RAISE_IF_NOT(!IsValid(), "Thread has already been created"); // 确保未创建过线程
            if constexpr (std::is_pointer_v<FType>)
            {
                if (nullptr == func)
                {
                    APPKIT_RAISE("Thread function pointer must not be null"); // 确保函数指针有效
                    return ErrorCode::INVALID_ARG;
                }
            }

            struct ThreadBase final
            {
                FType func_;
            };

            auto base = new ThreadBase(func);
            static auto base_func = +[](void *arg) -> void
            {
                auto *self = static_cast<ThreadBase *>(arg);
                self->func_();

                delete self;
                Exit_();
            };

            auto ret = this->Create_(base, base_func, name, stack_depth, priority);
            if (ret != ErrorCode::OK)
            {
                delete base;
            }

            return ret;
        }

        /**
         * @brief 检查线程是否可用
         * @return bool 线程句柄是否存在
         */
        [[nodiscard]] bool IsValid() const;

        /**
         * @brief 将对象转换为系统句柄
         */
        explicit operator ThreadId() const;
    };

    /**
     * @brief 当前线程函数命名空间
     */
    namespace this_thread
    {
        /**
         * @brief 获取当前线程
         * @return Thread 当前线程类
         */
        Thread Current();

        /**
         * @brief 线程休眠函数
         * @param interval 等待时间
         */
        void SleepFor(const Duration &interval);

        /**
         * @brief 线程休眠函数
         * @param last_time 参考时间点
         * @param interval 等待时间
         */
        void SleepUntil(TimePoint &last_time, const Duration &interval);

        /**
         * @brief 线程切换
         */
        void Yield();
    } // namespace this_thread
} // namespace appkit::osal