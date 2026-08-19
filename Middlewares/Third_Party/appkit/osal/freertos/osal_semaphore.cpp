#include <osal_semaphore.hpp>

namespace appkit::osal
{
	Semaphore::Semaphore(const uint32_t initial_value) : handle_(xSemaphoreCreateCounting(UINT32_MAX, initial_value))
	{
	}

	Semaphore::~Semaphore()
	{
		vSemaphoreDelete(handle_);
	}

	void Semaphore::Post()
	{
		xSemaphoreGive(handle_);
	}

	void Semaphore::PostFromCallback(bool in_isr)
	{
		if (in_isr)
		{
			BaseType_t task_woken = pdFALSE;
			xSemaphoreGiveFromISR(handle_, std::addressof(task_woken));
			portYIELD_FROM_ISR(task_woken);
		}
		else
		{
			xSemaphoreGive(handle_);
		}
	}

	[[nodiscard]] ErrorCode Semaphore::Wait(const Duration &timeout)
	{
		BaseType_t ret{};

		if (timeout >= gWaitForever)
		{
			ret = xSemaphoreTake(handle_, portMAX_DELAY);
		}
		else
		{
			ret = xSemaphoreTake(handle_, (DurationCast<SystemTimeUnit, TickType_t>(timeout)));
		}

		return ret == pdPASS ? ErrorCode::OK : ErrorCode::TIMEOUT;
	}

	[[nodiscard]] uint32_t Semaphore::GetValue()
	{
		return uxSemaphoreGetCount(handle_);
	}
} // namespace appkit::osal
