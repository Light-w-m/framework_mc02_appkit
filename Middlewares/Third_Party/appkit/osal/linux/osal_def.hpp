/**
 * @file osal_def.h
 * @brief OSAL定义头文件
 * @author dusk
 */
#pragma once

#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>

#include <common_type.hpp>
#include <common_time.hpp>

namespace appkit::osal
{
    using ThreadId = pthread_t;
    using SemaphoreId = sem_t;
    using MutexId = pthread_mutex_t;
    using QueueId = int[2];
}