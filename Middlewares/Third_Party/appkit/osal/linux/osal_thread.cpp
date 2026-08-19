/**
 * @file osal_thread.cpp
 * @brief OSAL线程实现文件
 * @author dusk
 */
#include <osal_thread.hpp>

#include <cstring>
#include <logger.hpp>

namespace appkit::osal
{
    struct ThreadBlock final
    {
        void (*func)(void *); ///< 线程函数指针
        void *arg;            ///< 线程函数参数指针
        char name[16];        ///< 线程名称

        /**
         * @brief 线程入口函数
         * @param arg 线程参数
         * @return 线程退出码
         */
        static void *Port(void *arg)
        {
            ThreadBlock *block = static_cast<ThreadBlock *>(arg);

            pthread_setname_np(pthread_self(), block->name);
            block->func(block->arg);

            delete block;
            return nullptr;
        }
    };

    FORCE_INLINE static sched_param GetRtosPriority(Thread::Priority priority)
    {
        int min_priority = sched_get_priority_min(SCHED_RR);
        int max_priority = sched_get_priority_max(SCHED_RR);
        APPKIT_RAISE_IF_NOT(max_priority - min_priority >= static_cast<int>(Thread::Priority::IDLE),
                            "Priority levels exceed system capabilities");

        sched_param param;
        param.sched_priority = min_priority + static_cast<int>(priority);
        return param;
    }

    ErrorCode Thread::Create_(void *thread_base, void (*func)(void *), const char *name, const size_t stack_depth,
                              const Priority priority)
    {
        const auto stack_size = (stack_depth + 3) / 4;
        const auto real_priority = GetRtosPriority(priority);

        pthread_attr_t attr;
        pthread_attr_init(&attr);

        pthread_attr_setstacksize(&attr, stack_size);

        // 优先尝试设置 SCHED_RR 和线程优先级
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_RR);

        if (pthread_attr_setschedparam(&attr, &real_priority) != 0)
        {
            APPKIT_LOG_WARNING("Failed to set thread priority. Falling back to default policy.");
            pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
            pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
        }

        // 创建线程
        ThreadBlock *block = new ThreadBlock{func, thread_base, {}};
        std::strncpy(block->name, name, sizeof(block->name) - 1);
        block->name[sizeof(block->name) - 1] = '\0';
        if (auto ret = pthread_create(&this->handle_, &attr, ThreadBlock::Port, block);
            ret != 0)
        {
            // 触发警告
            APPKIT_LOG_WARNING("Failed to create thread: %s (%s), retrying with default attributes.",
                               name, strerror(ret));

            // 完全使用系统默认属性（attr = nullptr）
            ret = pthread_create(&this->handle_, nullptr, ThreadBlock::Port, block);

            if (ret != 0)
            {
                APPKIT_LOG_ERROR("Failed to create thread: %s (%s)", name, strerror(ret));
                pthread_attr_destroy(&attr);
                delete block;
                return ErrorCode::FAILED;
            }
        }

        APPKIT_LOG_DEBUG("Thread <%s> created", name);
        pthread_attr_destroy(&attr);
        return ErrorCode::OK;
    }

    void Thread::Exit_()
    {
        pthread_exit(nullptr);
    }

    bool Thread::IsValid() const
    {
        return handle_ != ThreadId{};
    }

    Thread::operator ThreadId() const
    {
        return handle_;
    }

    namespace this_thread
    {
        Thread Current()
        {
            return Thread{pthread_self()};
        }

        void SleepFor(const Duration &interval)
        {
            struct timespec ts;
            ts.tv_sec = DurationCast<second, decltype(ts.tv_sec)>(interval);
            ts.tv_nsec = DurationCast<nanosecond, decltype(ts.tv_nsec)>(interval - Duration::From<second>(ts.tv_sec));
            UNUSED(clock_nanosleep(CLOCK_REALTIME, 0, &ts, nullptr));
        }

        void SleepUntil(TimePoint &last_time, const Duration &interval)
        {
            last_time = last_time + interval;

            struct timespec ts;
            ts.tv_sec = DurationCast<second, decltype(ts.tv_sec)>(last_time.SinceEpoch());
            ts.tv_nsec = DurationCast<nanosecond, decltype(ts.tv_nsec)>(last_time.SinceEpoch() - Duration::From<second>(ts.tv_sec));

            while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, &ts) && errno == EINTR)
            {
            }
        }

        void Yield()
        {
            sched_yield();
        }
    }
}