#include <osal_mutex.hpp>

#include <common_assert.hpp>

namespace appkit::osal
{
    Mutex::Mutex() : handle_(xSemaphoreCreateMutex())
    {
    }

    Mutex::~Mutex()
    {
        if (handle_ != nullptr)
        {
            vSemaphoreDelete(handle_);
        }
    }

    ErrorCode Mutex::Lock()
    {
        APPKIT_RAISE_IF(CheckInIsr(), "Cannot lock mutex in ISR context");

        if (const auto ret = xSemaphoreTake(handle_, portMAX_DELAY);
            ret == pdPASS)
        {
            return ErrorCode::OK;
        }

        return ErrorCode::FAILED;
    }

    ErrorCode Mutex::TryLock()
    {
        APPKIT_RAISE_IF(CheckInIsr(), "Cannot lock mutex in ISR context");

        if (const auto ret = xSemaphoreTake(handle_, 0); ret == pdPASS)
        {
            return ErrorCode::OK;
        }

        return ErrorCode::TIMEOUT;
    }

    void Mutex::Unlock()
    {
        APPKIT_RAISE_IF(CheckInIsr(), "Cannot lock mutex in ISR context");
        
        xSemaphoreGive(handle_);
    }

    bool Mutex::IsLockFetched() const
    {
        return xSemaphoreGetMutexHolder(handle_) == xTaskGetCurrentTaskHandle();
    }

    LockGuard::LockGuard(Mutex &mutex) : mutex_(mutex)
    {

        UNUSED(mutex_.Lock());
    }

    LockGuard::~LockGuard()
    {
        mutex_.Unlock();
    }

} // namespace appkit::osal
