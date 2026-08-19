#include <timebase.hpp>

#include <logger.hpp>

namespace appkit
{
	ErrorCode TimeBase::Open(const char *name)
	{
		if (nullptr == name) [[unlikely]]
		{
			return ErrorCode::INVALID_ARG;
		}

		if (nullptr != block_) [[unlikely]]
		{
			APPKIT_LOG_ERROR("Device already open");
			return ErrorCode::FAILED;
		}

		return Block::GetDeviceBlock(name, &block_);
	}

	TimePoint TimeBase::GetTime() const
	{
		if (nullptr == block_) [[unlikely]]
		{
			return Clock::steady_clock->Now();
		}

		return block_->Now();
	}

}
