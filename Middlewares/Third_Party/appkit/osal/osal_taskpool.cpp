#include <osal_taskpool.hpp>

namespace appkit::osal
{
    std::size_t TaskPool::ValidateCount_(const std::size_t value, const char *message)
    {
        if (value == 0)
        {
            APPKIT_RAISE(message);
        }
        return value;
    }

    TaskPool::TaskPool(const std::size_t worker_count, const std::size_t task_capacity)
        : worker_count_(ValidateCount_(worker_count, "TaskPool worker count must be greater than zero")),
          task_capacity_(ValidateCount_(task_capacity, "TaskPool capacity must be greater than zero")),
          task_queue_(task_capacity_),
          workers_(new Thread[worker_count_]{}),
          slots_(new TaskSlot[task_capacity_] {})
    {
        if (workers_ == nullptr || slots_ == nullptr)
        {
            APPKIT_RAISE("TaskPool memory allocation failed");
        }
    }

    TaskPool::~TaskPool()
    {
        if (const auto ret = Stop(); ret != ErrorCode::OK)
        {
            APPKIT_RAISE("TaskPool must not be destroyed from its worker or an interrupt context");
        }

        delete[] slots_;
        delete[] workers_;
    }

    ErrorCode TaskPool::Start(const char *name, const std::size_t stack_depth, const Thread::Priority priority)
    {
        if (CheckInIsr())
        {
            return ErrorCode::NOT_SUPPORTED;
        }
        if (name == nullptr || stack_depth == 0)
        {
            return ErrorCode::INVALID_ARG;
        }

        auto expected = State::CREATED;
        if (!state_.compare_exchange_strong(expected, State::STARTING,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire))
        {
            return ErrorCode::BUSY;
        }

        created_workers_ = 0;
        for (; created_workers_ < worker_count_; ++created_workers_)
        {
            const auto ret = workers_[created_workers_].Create(
                &TaskPool::WorkerEntry_, this, name, stack_depth, priority);
            if (ret != ErrorCode::OK)
            {
                state_.store(State::STOPPING, std::memory_order_release);
                StopWorkers_(created_workers_);
                state_.store(State::STOPPED, std::memory_order_release);
                return ret;
            }
        }

        state_.store(State::RUNNING, std::memory_order_release);
        return ErrorCode::OK;
    }

    ErrorCode TaskPool::Stop()
    {
        if (CheckInIsr())
        {
            return ErrorCode::NOT_SUPPORTED;
        }
        if (IsWorkerThread_())
        {
            return ErrorCode::BUSY;
        }

        auto current = state_.load(std::memory_order_acquire);
        for (;;)
        {
            if (current == State::STOPPED)
            {
                return ErrorCode::OK;
            }
            if (current == State::STARTING || current == State::STOPPING)
            {
                return ErrorCode::BUSY;
            }
            if (current == State::CREATED)
            {
                if (state_.compare_exchange_weak(current, State::STOPPED,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_acquire))
                {
                    return ErrorCode::OK;
                }
                continue;
            }
            if (state_.compare_exchange_weak(current, State::STOPPING,
                                             std::memory_order_acq_rel,
                                             std::memory_order_acquire))
            {
                break;
            }
        }

        while (active_submitters_.load(std::memory_order_acquire) != 0)
        {
            this_thread::Yield();
        }

        StopWorkers_(created_workers_);
        state_.store(State::STOPPED, std::memory_order_release);
        return ErrorCode::OK;
    }

    void TaskPool::StopWorkers_(const std::size_t count)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            TaskSlot *stop_token = nullptr;
            while (task_queue_.Push(stop_token, Duration::Max()) != ErrorCode::OK)
            {
                this_thread::Yield();
            }
        }

        for (std::size_t i = 0; i < count; ++i)
        {
            while (worker_exit_sem_.Wait(Duration::Max()) != ErrorCode::OK)
            {
            }
        }
    }

    void TaskPool::WorkerEntry_(TaskPool *self)
    {
        self->WorkerLoop_();
    }

    void TaskPool::WorkerLoop_()
    {
        for (;;)
        {
            TaskSlot *slot{};
            if (task_queue_.Pop(slot, Duration::Max()) != ErrorCode::OK)
            {
                continue;
            }

            if (slot == nullptr)
            {
                break;
            }

            slot->state.store(SlotState::RUNNING, std::memory_order_release);
            slot->invoke(slot->storage);
            slot->destroy(slot->storage);
            ReleaseSlot_(*slot);
            outstanding_tasks_.fetch_sub(1, std::memory_order_acq_rel);
        }

        worker_exit_sem_.Post();
    }

    bool TaskPool::IsWorkerThread_() const
    {
        if (created_workers_ == 0)
        {
            return false;
        }

        const auto current = static_cast<ThreadId>(this_thread::Current());
        for (std::size_t i = 0; i < created_workers_; ++i)
        {
            if (static_cast<ThreadId>(workers_[i]) == current)
            {
                return true;
            }
        }
        return false;
    }

    bool TaskPool::BeginSubmit_()
    {
        active_submitters_.fetch_add(1, std::memory_order_acq_rel);
        if (state_.load(std::memory_order_acquire) != State::RUNNING)
        {
            active_submitters_.fetch_sub(1, std::memory_order_acq_rel);
            return false;
        }
        return true;
    }

    void TaskPool::EndSubmit_()
    {
        active_submitters_.fetch_sub(1, std::memory_order_acq_rel);
    }

    TaskPool::TaskSlot *TaskPool::AcquireSlot_(const Duration &timeout, ErrorCode &error)
    {
        const bool wait_forever = timeout >= gWaitForever;
        const auto deadline = wait_forever || timeout <= Duration::Zero()
                                  ? TimePoint{}
                                  : Clock::system_clock->Now() + timeout;

        for (;;)
        {
            const auto start = slot_hint_.fetch_add(1, std::memory_order_relaxed) % task_capacity_;
            for (std::size_t offset = 0; offset < task_capacity_; ++offset)
            {
                auto &slot = slots_[(start + offset) % task_capacity_];
                auto expected = SlotState::FREE;
                if (slot.state.compare_exchange_strong(expected, SlotState::CONSTRUCTING,
                                                       std::memory_order_acq_rel,
                                                       std::memory_order_acquire))
                {
                    error = ErrorCode::OK;
                    return std::addressof(slot);
                }
            }

            if (timeout <= Duration::Zero())
            {
                error = ErrorCode::FULL;
                return nullptr;
            }
            if (state_.load(std::memory_order_acquire) != State::RUNNING)
            {
                error = ErrorCode::BUSY;
                return nullptr;
            }
            if (!wait_forever && Clock::system_clock->Now() >= deadline)
            {
                error = ErrorCode::TIMEOUT;
                return nullptr;
            }

            this_thread::Yield();
        }
    }

    void TaskPool::ReleaseSlot_(TaskSlot &slot) noexcept
    {
        slot.invoke = nullptr;
        slot.destroy = nullptr;
        slot.state.store(SlotState::FREE, std::memory_order_release);
    }

    bool TaskPool::IsRunning() const
    {
        return state_.load(std::memory_order_acquire) == State::RUNNING;
    }

    std::size_t TaskPool::WorkerCount() const
    {
        return worker_count_;
    }

    std::size_t TaskPool::Capacity() const
    {
        return task_capacity_;
    }

    std::size_t TaskPool::Pending() const
    {
        return outstanding_tasks_.load(std::memory_order_acquire);
    }

    std::size_t TaskPool::Available() const
    {
        std::size_t available = 0;
        for (std::size_t i = 0; i < task_capacity_; ++i)
        {
            if (slots_[i].state.load(std::memory_order_acquire) == SlotState::FREE)
            {
                ++available;
            }
        }
        return available;
    }
} // namespace appkit::osal
