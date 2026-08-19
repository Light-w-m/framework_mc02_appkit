/**
 * @file common_time.cpp
 * @brief 通用时间实现
 * @author dusk
 * @date 2025-12-26
 */
#include <common_time.hpp>

namespace appkit
{
	/**
	 * @brief 默认时钟类
	 * @note 该时钟类仅用于占位，实际应用中应由具体平台实现
	 */
	constinit static class DefaultClock final : public Clock
	{
	public:
		constexpr DefaultClock()
			: Clock(true)
		{
		}

		TimePoint Now() const override
		{
			return TimePoint{};
		}
	} g_default_clock;

	constinit const Clock *Clock::steady_clock = &g_default_clock;
	constinit const Clock *Clock::system_clock = &g_default_clock;
}
