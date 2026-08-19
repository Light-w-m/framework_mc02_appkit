#pragma once

#include <macros.hpp>
#include <common_cb.hpp>

namespace appkit
{
    using FatalCallback = Callback<const char *, int, const char *>;

    class Assert final
    {
        DECL_STATIC_PROPERTY(private, FatalCallback, fatalCallback, {}) ///< 致命错误回调

    public:
        /**
         * @brief 大小限制模式枚举定义
         */
        enum class SizeLimitMode
        {
            LESS,  ///< 小于指定大小
            EQUAL, ///< 等于指定大小
            GREAT, ///< 大于指定大小
        };

        /**
         * @brief 获取致命错误回调引用
         * @return 致命错误回调引用
         */
        FORCE_INLINE static const FatalCallback &GetFatalCallback()
        {
            return PROPERTY(fatalCallback);
        }

        /**
         * @brief 设置致命错误回调
         * @param cb 致命错误回调
         */
        FORCE_INLINE static void SetFatalCallback(const FatalCallback &cb)
        {
            PROPERTY(fatalCallback) = cb;
        }

        /**
         * @brief 触发致命错误
         * @param file 文件名
         * @param line 行号
         * @param func 函数名
         * @param expr 断言表达式
         */
        [[noreturn]] static void TriggerFatal(bool in_isr, const char *file, int line, const char *msg);

        /**
         * @brief 大小限制断言
         * @tparam mode 大小限制模式
         * @param size 实际大小
         * @param expected_size 期望大小
         * @param msg 错误消息
         * @param file 文件名
         * @param line 行号
         */
        template <SizeLimitMode mode = SizeLimitMode::EQUAL>
        static void SizeLimit(size_t size, size_t expected_size, const char *file, int line, const char *msg = "")
        {
#ifdef APPKIT_STRICT_MODE
            if constexpr (mode == SizeLimitMode::LESS)
            {
                if (size >= expected_size) [[unlikely]]
                {
                    TriggerFatal(false, file, line, msg);
                }
            }
            else if constexpr (mode == SizeLimitMode::EQUAL)
            {
                if (size != expected_size) [[unlikely]]
                {
                    TriggerFatal(false, file, line, msg);
                }
            }
            else if constexpr (mode == SizeLimitMode::GREAT)
            {
                if (size <= expected_size) [[unlikely]]
                {
                    TriggerFatal(false, file, line, msg);
                }
            }
#else
            UNUSED(size);
            UNUSED(expected_size);
            UNUSED(file);
            UNUSED(line);
            UNUSED(msg);
#endif
        }

        /**
         * @brief 条件断言
         * @param condition 条件表达式
         * @param msg 错误消息
         * @param file 文件名
         * @param line 行号
         */
        static void Check(bool condition, const char *file, int line, const char *msg)
        {
#ifdef APPKIT_STRICT_MODE
            if (!condition) [[unlikely]]
            {
                TriggerFatal(false, file, line, msg);
            }
#else
            UNUSED(condition);
            UNUSED(file);
            UNUSED(line);
            UNUSED(msg);
#endif
        }
    };
}

/**
 * @brief 触发致命错误宏定义
 * @param msg 错误消息
 */
#define APPKIT_RAISE(msg) appkit::Assert::TriggerFatal(false, __PROJECT_FILE_NAME__, __LINE__, (msg))

/**
 * @brief 从回调函数触发致命错误宏定义
 * @param in_isr 是否在中断服务例程中
 * @param msg 错误消息
 */
#define APPKIT_RAISE_FROM_CALLBACK(in_isr, msg) appkit::Assert::TriggerFatal((in_isr), __PROJECT_FILE_NAME__, __LINE__, (msg))

/**
 * @brief 条件断言宏定义
 * @param cond 条件表达式
 * @param msg 错误消息
 */
#define APPKIT_RAISE_IF(cond, msg) appkit::Assert::Check(!(cond), __PROJECT_FILE_NAME__, __LINE__, (msg))

/**
 * @brief 条件断言宏定义
 * @param cond 条件表达式
 * @param msg 错误消息
 */
#define APPKIT_RAISE_IF_NOT(cond, msg) appkit::Assert::Check((cond), __PROJECT_FILE_NAME__, __LINE__, (msg))

/**
 * @brief 大小限制断言宏定义
 * @tparam mode 大小限制模式
 * @param size 实际大小
 * @param expected_size 期望大小
 * @param msg 错误消息
 */
#define SizeLimitAssert(mode, size, expected_size, msg) \
    appkit::Assert::SizeLimit<(mode)>((size), (expected_size), __PROJECT_FILE_NAME__, __LINE__, (msg))
