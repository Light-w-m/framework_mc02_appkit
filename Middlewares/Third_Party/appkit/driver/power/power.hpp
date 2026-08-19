#pragma once

#include <common_type.hpp>
#include <driver.hpp>

namespace appkit
{
    class PowerManager final
    {
    public:
        class Block : public Driver<Block>
        {
        public:
            enum class Status : uint8_t
            {
                Reset = 0,
                Shutdown,
                JumpToBootloader
            };

            virtual ~Block() = default;

            virtual void ChangeStatus(Status status) = 0;
        };

        /**
         * @brief 构造函数
         */
        constexpr PowerManager()
        {
        }

        /**
         * @brief 打开电源管理设备
         * @param name 设备名称
         * @return 错误码
         */
        ErrorCode Open(const char *name);

        /**
         * @brief 重置系统
         */
        void Reset() const;

        /**
         * @brief 关闭系统
         */
        void Shutdown() const;

        /**
         * @brief 跳转到引导加载程序
         */
        void JumpToBootloader() const;

    private:
        Block *block_{}; ///< 电源管理块指针
    };
}
