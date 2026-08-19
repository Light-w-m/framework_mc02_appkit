#include <osal_mutex.hpp>

#include <common_assert.hpp>

namespace appkit::osal
{
    Mutex::Mutex() : handle_(PTHREAD_MUTEX_INITIALIZER)
    {
        APPKIT_RAISE_IF_NOT(pthread_mutex_init(&handle_, nullptr) == 0, "Mutex initialization failed");
    }

    Mutex::~Mutex()
    {
        pthread_mutex_destroy(&handle_);
    }

    ErrorCode Mutex::Lock()
    {
        if (pthread_mutex_lock(&handle_) == 0)
        {
            return ErrorCode::OK;
        }
        return ErrorCode::FAILED;
    }

    ErrorCode Mutex::TryLock()
    {
        if (pthread_mutex_trylock(&handle_) == 0)
        {
            return ErrorCode::OK;
        }
        return ErrorCode::FAILED;
    }

    void Mutex::Unlock()
    {
        pthread_mutex_unlock(&handle_);
    }

    bool Mutex::IsLockFetched() const
    {
        // POSIX mutex does not support checking if the current thread holds the lock
        return false;
    }

    LockGuard::LockGuard(Mutex &mutex) : mutex_(mutex)
    {
        UNUSED(mutex_.Lock());
    }

    LockGuard::~LockGuard()
    {
        mutex_.Unlock();
    }

}