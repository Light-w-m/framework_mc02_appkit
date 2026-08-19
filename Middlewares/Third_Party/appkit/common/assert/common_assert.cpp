/**
 * @file common_assert.cpp
 * @brief 通用断言实现
 * @author dusk
 * @date 2025-12-26
 */
#include <common_assert.hpp>

namespace appkit
{
    /**
     * @brief 平台相关的错误处理函数声明
     */
    extern void PlatformErrorHandler(bool in_isr, const char *file, int line, const char *msg);

    [[noreturn]] void Assert::TriggerFatal(bool in_isr, const char *file, int line, const char *msg)
    {
        // 调用用户注册的致命错误回调函数
        if (auto &cb = PROPERTY(fatalCallback);
            !cb.Empty())
        {
            cb.Call(in_isr, file, line, msg);
        }

        // 调用平台相关的错误处理函数
        PlatformErrorHandler(in_isr, file, line, msg);

        // 无限循环，等待重启
        while (true)
        {
        }
    }
}