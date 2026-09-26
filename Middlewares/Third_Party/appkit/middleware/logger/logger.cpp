#include <logger.hpp>

#include <common_esc.hpp>
#include <osal_thread.hpp>

#ifdef APPKIT_USE_DEFAULT_LOGGER_LEVEL
#pragma message "Using default logger level APPKIT_LOGGER_LEVEL INFO"
#endif

#ifdef APPKIT_USE_DEFAULT_LOGGER_OUTPUT_LEVEL
#pragma message "Using default logger output level APPKIT_LOGGER_OUTPUT_LEVEL WARNING"
#endif

namespace appkit
{
#if defined(APPKIT_LOGGER_PUBLISH_MSG) && (APPKIT_LOGGER_PUBLISH_MSG == 1)
    Topic::Domain Logger::domain_{"sys"};
    Topic Logger::topic_{Topic::CreateTopic<LogData>("log", &domain_, true, true)};
#endif

#ifdef APPKIT_LOGGER_BUFFER_SIZE
    const char *Logger::GetColor(const Level level)
    {
        static const constinit auto max_index = 4;
        static constexpr const char *const color_map[]{
            Color::GetStr(Color::Type::RED),    // ERROR
            Color::GetStr(Color::Type::YELLOW), // WARNING
            Color::GetStr(Color::Type::CYAN),   // INFO
            Color::GetStr(Color::Type::MAGENTA) // DEBUG
        };

        if (const auto index = std::to_underlying(level);
            index <= max_index)
        {
            return color_map[index];
        }

        return Color::GetStr(Color::Type::NONE);
    }

    const char *Logger::LevelToStr(const Level level)
    {
        static const constinit auto max_index = 4;
        static const char *const level_map[]{
            "E", // ERROR
            "W", // WARNING
            "I", // INFO
            "D"  // DEBUG
        };

        if (const auto index = std::to_underlying(level);
            index <= max_index)
        {
            return level_map[index];
        }

        return "UNKNOWN";
    }
#endif

#ifdef APPKIT_LOGGER_BUFFER_SIZE
    void Logger::Publish(const Level level, const char *file, int line, const char *fmt, ...)
#else
    void Logger::Publish(const Level level, const char *file, int line)
#endif
    {
        LogData data; ///< 日志信息结构体，可不需要进行初始化（节省时间）
        data.timestamp = DurationCast<second>(Clock::system_clock->Now().SinceEpoch());
        data.level = level;
        data.file = file;
        data.line = line;

#ifdef APPKIT_LOGGER_BUFFER_SIZE
        va_list args;
        va_start(args, fmt);
        const auto ret = vsnprintf(data.buffer, APPKIT_LOGGER_BUFFER_SIZE, fmt, args);
        va_end(args);

        if (ret < 0)
        {
            // 输出错误信息
            APPKIT_RAISE("Log message generate error: format string error");
        }
        else [[likely]]
#endif
        {
#if defined(APPKIT_LOGGER_PUBLISH_MSG) && (APPKIT_LOGGER_PUBLISH_MSG == 1)
            // 日志信息发布到话题中
            UNUSED(topic_.Publish(data));
#endif

#ifdef APPKIT_LOGGER_BUFFER_SIZE
            // 打印到终端
            PrintToTerminal(data);
#endif
        }
    }

#ifdef APPKIT_LOGGER_BUFFER_SIZE
    void Logger::PrintToTerminal(const LogData &msg)
    {
        const auto &[timestamp, level, file, line, buffer] = msg;

        // 根据日志级别决定是否输出
        if (std::to_underlying(static_cast<Level>(APPKIT_LOGGER_OUTPUT_LEVEL_VALUE)) >= std::to_underlying(level))
        {
            if (STDIO::write_ && STDIO::write_->Writable())
            {
                STDIO::Printf("%s%s [%u](%s:%u) %s%s\r\n",
                              GetColor(level), LevelToStr(level),
                              timestamp, file, line,
                              buffer, Format::GetStr(Format::Type::NORMAL));
            }
        }
    }
#endif
}
