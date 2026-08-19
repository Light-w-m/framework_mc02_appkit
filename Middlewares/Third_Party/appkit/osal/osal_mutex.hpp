#pragma once

#include <osal_def.hpp>

namespace appkit::osal
{
    /**
     * @brief 互斥量类
     */
    class Mutex final
    {
    private:
        MutexId handle_{}; ///< 互斥量句柄

        friend class LockGuard;

    public:
        /**
         * @brief 构造函数
         */
        Mutex();

        /**
         * @brief 析构函数
         */
        ~Mutex();

        /**
         * @brief 获取锁
         * @return ErrorCode 锁状态
         */
        [[nodiscard]] ErrorCode Lock();

        /**
         * @brief 尝试获取锁
         * @return ErrorCode 锁状态
         */
        [[nodiscard]] ErrorCode TryLock();

        /**
         * @brief 释放锁
         */
        void Unlock();

        /**
         * @brief 检查锁是否被当前线程持有
         * @return bool 锁是否被当前线程持有
         */
        [[nodiscard]] bool IsLockFetched() const;
    };

    /**
     * @brief 互斥量锁自动管理类
     */
    class LockGuard final
    {
        DECL_COPY_DISABLE(LockGuard)
        DECL_MOVE_DISABLE(LockGuard)

    private:
        Mutex &mutex_; ///< 互斥量引用

    public:
        /**
         * @brief 构造函数，自动获取锁
         * @param mutex 互斥量引用
         */
        explicit LockGuard(Mutex &mutex);

        /**
         * @brief 析构函数，自动释放锁
         */
        ~LockGuard();
    };
}