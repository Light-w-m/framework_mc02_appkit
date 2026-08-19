#pragma once

#include <common_rw.hpp>
#include <osal_mutex.hpp>

#if defined(APPKIT_LOGGER_PUBLISH_MSG) && (APPKIT_LOGGER_PUBLISH_MSG == 1)
#include <message.hpp>
#endif

#ifdef APPKIT_LOGGER_BUFFER_SIZE
/**
 * @brief 检查日志缓冲区大小是否有效
 */
static_assert(APPKIT_LOGGER_BUFFER_SIZE > 0, "APPKIT_LOGGER_BUFFER_SIZE must be greater than 0");
#endif

#ifndef APPKIT_LOGGER_LEVEL
/**
 * @brief 默认日志级别为DEBUG
 * @note 可选值为0-ERROR，1-WARNING，2-INFO，3-DEBUG
 */
#define APPKIT_LOGGER_LEVEL INFO
#define APPKIT_USE_DEFAULT_LOGGER_LEVEL
#endif

#ifndef APPKIT_LOGGER_OUTPUT_LEVEL
/**
 * @brief 默认日志输出级别为DEBUG
 * @note 可选值为0-ERROR，1-WARNING，2-INFO，3-DEBUG
 */
#define APPKIT_LOGGER_OUTPUT_LEVEL WARNING
#define APPKIT_USE_DEFAULT_LOGGER_OUTPUT_LEVEL
#endif

#ifdef DEBUG
#undef DEBUG
#endif

namespace appkit
{
    /**
     * @brief 日志类
     */
    class Logger final
    {
    public:
        /**
         * @brief 日志级别
         */
        enum class Level : uint8_t
        {
            ERROR = 0,   ///< 错误
            WARNING = 1, ///< 警告
            INFO = 2,    ///< 信息
            DEBUG = 3,   ///< 调试
        };

    private:
#ifdef APPKIT_LOGGER_BUFFER_SIZE
        /**
         * @brief 获取日志级别对应的颜色代码
         * @param level 日志级别
         * @return 颜色代码字符串
         */
        static const char *GetColor(Level level);

        /**
         * @brief 将日志级别转换为字符串
         * @param level 日志级别
         * @return 日志级别字符串
         */
        static const char *LevelToStr(Level level);
#endif

    public:
        /**
         * @brief 日志数据结构
         */
        struct LogData
        {
            uint32_t timestamp; ///< 时间戳
            Level level;        ///< 日志级别
            const char *file;   ///< 文件名
            int line;           ///< 行号
#ifdef APPKIT_LOGGER_BUFFER_SIZE
            char buffer[APPKIT_LOGGER_BUFFER_SIZE]; ///< 日志缓冲区, 存储格式化后的日志内容
#endif
        };

        static_assert(std::is_standard_layout_v<LogData>, "LogData must be a standard layout type");

    private:
#if defined(APPKIT_LOGGER_PUBLISH_MSG) && (APPKIT_LOGGER_PUBLISH_MSG == 1)
        static Topic::Domain domain_; ///< 日志主题域
        static Topic topic_;          ///< 日志主题
#endif

#ifdef APPKIT_LOGGER_BUFFER_SIZE
        /**
         * @brief 打印到终端
         * @param msg 日志数据
         * @note 仅在启用日志缓冲区时可用
         */
        static void PrintToTerminal(const LogData &msg);
#endif

    public:
#ifdef APPKIT_LOGGER_BUFFER_SIZE
        /**
         * @brief 发布日志
         * @param level 日志级别
         * @param file 文件名
         * @param line 行号
         * @param fmt 格式化字符串
         * @param ... 可变参数
         */
        static void Publish(Level level, const char *file, int line, const char *fmt, ...);
#else
        /**
         * @brief 发布日志
         * @param level 日志级别
         * @param file 文件名
         * @param line 行号
         */
        static void Publish(Level level, const char *file, int line);
#endif
    };
}

/**
 * @brief 日志宏定义
 * @param level 日志级别
 * @param fmt 格式化字符串
 * @param ... 可变参数
 * @note 不应该直接使用此宏，应使用APPKIT_LOG_DEBUG、APPKIT_LOG_INFO、APPKIT_LOG_WARNING、APPKIT_LOG_ERROR等宏
 */
#ifdef APPKIT_LOGGER_BUFFER_SIZE
#define APPKIT_LOG(level, fmt, ...) \
    appkit::Logger::Publish(appkit::Logger::Level::level, __PROJECT_FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define APPKIT_LOG(level, fmt, ...) \
    appkit::Logger::Publish(appkit::Logger::Level::level, __PROJECT_FILE_NAME__, __LINE__)
#endif

#define DEBUG 3
#define INFO 2
#define WARNING 1
#define ERROR 0

/**
 * @brief 调试日志宏定义
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
#if APPKIT_LOGGER_LEVEL >= 3
#define APPKIT_LOG_DEBUG(fmt, ...) APPKIT_LOG(DEBUG, fmt, ##__VA_ARGS__)
#else
#define APPKIT_LOG_DEBUG(fmt, ...)
#endif

/**
 * @brief 信息日志宏定义
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
#if APPKIT_LOGGER_LEVEL >= 2
#define APPKIT_LOG_INFO(fmt, ...) APPKIT_LOG(INFO, fmt, ##__VA_ARGS__)
#else
#define APPKIT_LOG_INFO(fmt, ...)
#endif

/**
 * @brief 警告日志宏定义
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
#if APPKIT_LOGGER_LEVEL >= 1
#define APPKIT_LOG_WARNING(fmt, ...) APPKIT_LOG(WARNING, fmt, ##__VA_ARGS__)
#else
#define APPKIT_LOG_WARNING(fmt, ...)
#endif

/**
 * @brief 错误日志宏定义
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
#if APPKIT_LOGGER_LEVEL >= 0
#define APPKIT_LOG_ERROR(fmt, ...) APPKIT_LOG(ERROR, fmt, ##__VA_ARGS__)
#else
#define APPKIT_LOG_ERROR(fmt, ...)
#endif

#undef DEBUG
#undef INFO
#undef WARNING
#undef ERROR
