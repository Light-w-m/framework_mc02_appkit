#pragma once

#include <ramfs.hpp>
#include <common_time.hpp>

#include <driver.hpp>

namespace appkit
{
    class TimeBase final
    {
    public:
        class Block : public Clock, public Driver<Block>
        {
        public:
            /**
             * @brief 构造函数
             * @param steady 是否为稳定时钟
             */
            using Clock::Clock;
        };

        constexpr explicit TimeBase() = default;

        ErrorCode Open(const char *name);

        [[nodiscard]] TimePoint GetTime() const; ///< 获取当前时间戳

    private:
        Block *block_{nullptr};      ///< 时间基准块指针
    };
}
