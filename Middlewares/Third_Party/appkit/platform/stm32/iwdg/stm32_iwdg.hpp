#pragma once

#include <main.h>

#ifdef HAL_IWDG_MODULE_ENABLED

#include <iwdg.h>

#include <watchdog.hpp>
#include <osal_thread.hpp>

namespace appkit::stm32
{
    class STM32Iwdg : public Watchdog::Block
    {
        IWDG_HandleTypeDef *handle_; ///< IWDG句柄
        osal::Thread thread_{};      ///< 看门狗喂狗线程

        /**
         * @brief 看门狗喂狗线程函数
         * @param self STM32Iwdg指针
         */
        static void ThreadFunc(STM32Iwdg *self);

    public:
        /**
         * @brief 构造函数
         * @param name 设备名称
         * @param handle IWDG句柄
         * @param stack_depth 线程栈深度
         */
        explicit STM32Iwdg(const char *name, IWDG_HandleTypeDef &handle, size_t stack_depth = 512);
    };
}

#endif
