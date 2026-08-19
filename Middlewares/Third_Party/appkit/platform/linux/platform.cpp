#include <platform.hpp>

#include <common_rw.hpp>
#include <common_time.hpp>
#include <common_assert.hpp>

#include <osal_thread.hpp>
#include <logger.hpp>

#include <cstring>

#include <bits/types/FILE.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#ifndef APPKIT_PRINTF_BUFFER_SIZE
#pragma message "APPKIT_PRINTF_BUFFER_SIZE is not defined, STDIO read port will be disabled"
#endif

namespace appkit
{
    class SteadyClock final : public Clock
    {
    public:
        constexpr SteadyClock()
            : Clock(true)
        {
        }

        TimePoint Now() const override
        {
            thread_local struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            return TimePoint{Duration::From<second>(ts.tv_sec) + Duration::From<nanosecond>(ts.tv_nsec)};
        }
    };

#ifdef APPKIT_PRINTF_BUFFER_SIZE

    static osal::Thread StdiThreadHandle{};

    void StdiThread(ReadPort *read_port)
    {
        using namespace appkit::time_literals;

        static uint8_t read_buff[static_cast<size_t>(4 * APPKIT_PRINTF_BUFFER_SIZE)];

        if (!isatty(STDIN_FILENO))
        {
            APPKIT_LOG_WARNING("STDIO.read_: stdin is not a TTY, parking thread forever");
            while (true)
            {
                osal::this_thread::SleepFor(1_h);
            }
        }

        while (true)
        {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(STDIN_FILENO, &rfds);

            int ret = select(STDIN_FILENO + 1, &rfds, NULL, NULL, NULL);

            if (ret > 0 && FD_ISSET(STDIN_FILENO, &rfds))
            {
                int ready = 0;
                if (ioctl(STDIN_FILENO, FIONREAD, &ready) != -1 && ready > 0)
                {
                    auto size = fread(read_buff, sizeof(char), ready, stdin);
                    if (size < 1)
                    {
                        continue;
                    }
                    read_port->buffer_->PushBatch(read_buff, size);
                    read_port->ProcessPendingReads(false);
                }
            }
        }
    }

#endif

    ErrorCode StdoFunc(WritePort &write_port)
    {
        WriteInfo info;

        if (!Check(write_port.info_buffer_->Pop(info)))
        {
            return ErrorCode::FAILED;
        }

        auto write_size = fwrite(info.data.GetData<char>(), sizeof(char), info.data.GetSize(), stdout);
        fflush(stdout);
        write_port.Finish(
            false, write_size == info.data.GetSize() ? ErrorCode::OK : ErrorCode::FAILED, info,
            write_size);

        return ErrorCode::OK;
    }

    /**
     * @brief 获取平台名称
     * @return 平台名称字符串
     */
    const char *Platform::GetPlatformName()
    {
        return "Linux";
    }

    /**
     * @brief 获取平台唯一标识符
     * @return 平台唯一标识符字符串
     */
    const HashString<128> &Platform::GetPlatformUuid()
    {
        return platform_uuid_;
    }

    /**
     * @brief 平台初始化函数
     */
    ErrorCode Platform::Init()
    {
        // 初始化时钟
        {
            static constinit SteadyClock steady_clock{};
            Clock::steady_clock = std::addressof(steady_clock);
            APPKIT_LOG_DEBUG("Steady clock initialized for Linux platform");
        }

        // 配置全局读写接口
        {
            static ReadPort read_port(1024);
            static WritePort write_port(1024);

            read_port = [](ReadPort &port)
            {
                UNUSED(port);
                return ErrorCode::FAILED;
            };
            write_port = StdoFunc;

            STDIO::read_ = &read_port;
            STDIO::write_ = &write_port;

#ifdef APPKIT_PRINTF_BUFFER_SIZE
            // 启动读线程
            StdiThreadHandle.Create(StdiThread, &read_port, "StdiThread", 4096, osal::Thread::Priority::LOW);
#endif

            APPKIT_LOG_DEBUG("STDIO initialized with Linux platform read/write ports");
        }

        // 获取平台唯一标识符
        {
            FILE *file{};
            file = fopen("/sys/class/dmi/id/product_uuid", "r");
            if (file == NULL)
            {
                APPKIT_LOG_WARNING("Error opening file /sys/class/dmi/id/product_uuid");
                file = fopen("/etc/machine-id", "r");
                if (file == NULL)
                {
                    APPKIT_LOG_ERROR("Error opening file /etc/machine-id");
                    return ErrorCode::FAILED;
                }
            }

            char uuid_str[37];
            uint8_t uuid_len = 36;
            if (fgets(uuid_str, 37, file) != NULL)
            {
                uuid_len = strcspn(uuid_str, "\n");
                uuid_str[uuid_len] = '\0'; // 移除换行符
            }

            platform_uuid_ = HashString<128>({uuid_str, uuid_len});
            fclose(file);

            APPKIT_LOG_DEBUG("Platform UUID: %s", uuid_str);
        }

        return ErrorCode::OK;
    }

    void PlatformErrorHandler(bool in_isr, const char *file, int line, const char *msg)
    {
        // Linux平台特定的错误处理逻辑
        fprintf(stderr, "PlatformErrorHandler invoked%s: %s (%s:%d)\n", (in_isr ? " (in ISR)" : ""), msg, file, line);
        abort();
    }
}