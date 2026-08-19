#include <osal_queue.hpp>

#include <logger.hpp>

namespace appkit::osal
{
    QueueBase::QueueBase(std::size_t item_size, std::size_t max_items)
        : handle_(xQueueCreate(max_items, item_size))
    {
        if (handle_ == nullptr)
        {
            APPKIT_LOG_ERROR("Failed to create queue with item size %zu and max items %zu.", item_size, max_items);
            APPKIT_RAISE("Queue creation failed.");
        }
    }

    QueueBase::~QueueBase()
    {
        if (handle_ != nullptr)
        {
            vQueueDelete(handle_);
            handle_ = nullptr;
        }
    }

    ErrorCode QueueBase::Send(ConstRawData data, const Duration &timeout)
    {
        if (data.GetSize() == 0 || data.GetData<void>() == nullptr)
        {
            return ErrorCode::INVALID_ARG;
        }

        if (CheckInIsr())
        {
            BaseType_t task_woken = pdFALSE;
            if (xQueueSendToBackFromISR(handle_, data.GetData<void>(), std::addressof(task_woken)) != pdPASS)
            {
                return ErrorCode::TIMEOUT;
            }

            portYIELD_FROM_ISR(task_woken);
        }
        else
        {
            if (xQueueSendToBack(handle_, data.GetData<void>(), (appkit::DurationCast<SystemTimeUnit, TickType_t>(timeout))) != pdPASS)
            {
                return ErrorCode::TIMEOUT;
            }
        }

        return ErrorCode::OK;
    }

    ErrorCode QueueBase::Receive(RawData data, const Duration &timeout)
    {
        if (data.GetSize() == 0 || data.GetData<void>() == nullptr)
        {
            return ErrorCode::INVALID_ARG;
        }

        if (CheckInIsr())
        {
            BaseType_t task_woken = pdFALSE;
            if (xQueueReceiveFromISR(handle_, data.GetData<void>(), std::addressof(task_woken)) != pdPASS)
            {
                return ErrorCode::TIMEOUT;
            }

            portYIELD_FROM_ISR(task_woken);
        }
        else
        {
            if (xQueueReceive(handle_, data.GetData<void>(), (appkit::DurationCast<SystemTimeUnit, TickType_t>(timeout))) != pdPASS)
            {
                return ErrorCode::TIMEOUT;
            }
        }

        return ErrorCode::OK;
    }

    std::size_t QueueBase::Size() const
    {
        return uxQueueMessagesWaiting(handle_);
    }

    std::size_t QueueBase::Capacity() const
    {
        return uxQueueSpacesAvailable(handle_) + uxQueueMessagesWaiting(handle_);
    }

    std::size_t QueueBase::Available() const
    {
        return uxQueueSpacesAvailable(handle_);
    }
} // namespace appkit::osal
