#include <random.hpp>

#include <logger.hpp>

#include <numeric>
#include <random>

namespace appkit
{
	ErrorCode Random::Open(const char *name)
	{
		if (nullptr == name) [[unlikely]]
		{
            APPKIT_LOG_ERROR("Invalid device name");
			return ErrorCode::INVALID_ARG;
		}

		if (nullptr != block_) [[unlikely]]
		{
			APPKIT_LOG_ERROR("Device already open");
			return ErrorCode::FAILED;
		}

		return Block::GetDeviceBlock(name, &block_);
	}

	size_t Random::GetRandom(const size_t min, const size_t max) const
	{
		if (nullptr == block_) [[unlikely]]
		{
			APPKIT_LOG_ERROR("Device not open");
			return 0;
		}

		return block_->GetRawNum() % (max - min + 1) + min;
	}

	float Random::GetRandom(const float min, const float max) const
	{
		if (nullptr == block_) [[unlikely]]
		{
			APPKIT_LOG_ERROR("Device not open");
			return 0;
		}

		return static_cast<float>(block_->GetRawNum()) / static_cast<float>(std::numeric_limits<size_t>::max()) * (max - min) + min;
	}
}
