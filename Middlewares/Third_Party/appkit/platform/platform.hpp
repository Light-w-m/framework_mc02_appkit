#pragma once

#include <macros.hpp>
#include <hash_string.hpp>

namespace appkit
{
    /**
     * @brief 平台相关定义
     */
    class Platform
    {
    private:
        inline static HashString<128> platform_uuid_{};

    public:
        /**
         * @brief 获取平台名称
         * @return 平台名称字符串
         */
        static const char *GetPlatformName();

        /**
         * @brief 获取平台唯一标识符
         * @return 平台唯一标识符字符串
         */
        static const HashString<128> &GetPlatformUuid();

        /**
         * @brief 平台初始化函数
         */
        static ErrorCode Init();
    };
}