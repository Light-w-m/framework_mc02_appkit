#include <platform.hpp>

#include <main.h>

#include <logger.hpp>
#include <cstring>

namespace appkit
{
    /**
     * @brief 获取平台名称
     * @return 平台名称字符串
     */
    const char *Platform::GetPlatformName()
    {
        return "STM32";
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
        // 获取平台唯一标识符
        {
            char uuid_str[128];
            std::snprintf(uuid_str, sizeof(uuid_str), "%08lX%08lX%08lX", HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2());
            platform_uuid_ = HashString<128>({uuid_str, 24});

            APPKIT_LOG_DEBUG("Platform UUID: %s", uuid_str);
        }

        return ErrorCode::OK;
    }

    void PlatformErrorHandler(bool in_isr, const char *file, int line, const char *msg)
    {
        // STM32平台特定的错误处理逻辑
        UNUSED(in_isr);
        UNUSED(file);
        UNUSED(line);
        UNUSED(msg);

        __disable_irq();
        for (;;)
        {
        }
    }
}