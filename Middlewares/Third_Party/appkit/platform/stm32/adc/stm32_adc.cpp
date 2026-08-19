#include <stm32_adc.hpp>

#ifdef HAL_ADC_MODULE_ENABLED

#include <logger.hpp>

namespace appkit::stm32
{
    STM32ADC::Channel::Channel(const char *name, STM32ADC &parent, size_t index, float scale, float offset)
        : parent_(parent), index_(index), scale_(scale), offset_(offset)
    {
        APPKIT_RAISE_IF_NOT(index < parent.channel_count_, "ADC channel index out of range");

        // 注册设备
        APPKIT_RAISE_IF_NOT(Check(RegisterDevice(name)), "Register ADC channel failed");
    }

    Result<float> STM32ADC::Channel::Read()
    {
        // 使用DMA时，直接从缓存读取
        float sum = 0.0f;
        auto buffer = parent_.channel_buffer_.GetData<uint16_t>();

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
        SCB_InvalidateDCache_by_Addr(buffer, parent_.channel_buffer_.GetSize());
#endif

        for (size_t i = 0; i < parent_.filter_size_; i++)
        {
            sum += static_cast<float>(buffer[i * parent_.channel_count_ + index_]);
        }
        float value = (sum / parent_.filter_size_) / parent_.resolution_ * parent_.vref_ * scale_ + offset_;

        return Result<float>::Ok(value);
    }

    STM32ADC::STM32ADC(ADC_HandleTypeDef &handle, RawData buffer, float vref, size_t filter_size)
        : handle_(&handle), vref_(vref), filter_size_(filter_size), channel_buffer_(buffer)
    {
        APPKIT_RAISE_IF_NOT(filter_size > 0, "Filter size must be greater than 0");

        // 检查是否支持DMA
        APPKIT_RAISE_IF_NOT(handle_->DMA_Handle != nullptr, "ADC does not support DMA");

        // 设置DMA为循环模式
        handle_->DMA_Handle->Init.Mode = DMA_CIRCULAR;
        APPKIT_RAISE_IF_NOT(HAL_DMA_Init(handle_->DMA_Handle) == HAL_OK, "ADC DMA reinit failed");

        // 计算分辨率
        switch (handle_->Init.Resolution)
        {
        case ADC_RESOLUTION_16B:
            resolution_ = 65535.0f;
            break;
        case ADC_RESOLUTION_12B:
            resolution_ = 4095.0f;
            break;
        case ADC_RESOLUTION_10B:
            resolution_ = 1023.0f;
            break;
        case ADC_RESOLUTION_8B:
            resolution_ = 255.0f;
            break;
        case ADC_RESOLUTION_6B:
            resolution_ = 63.0f;
            break;
        default:
            APPKIT_RAISE("ADC resolution setting error");
            break;
        }

        // 计算通道数量
        channel_count_ = handle_->Init.NbrOfConversion;
        if (channel_count_ * filter_size_ * sizeof(uint16_t) > buffer.GetSize())
        {
            APPKIT_LOG_ERROR("ADC buffer size error: need %u, got %zu", channel_count_ * filter_size_ * sizeof(uint16_t),
                             buffer.GetSize());
            APPKIT_RAISE("ADC buffer size error");
        }

        // 校准ADC
        APPKIT_RAISE_IF_NOT(HAL_ADCEx_Calibration_Start(handle_, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) == HAL_OK, "ADC OFFSET calibration failed");
        APPKIT_RAISE_IF_NOT(HAL_ADCEx_Calibration_Start(handle_, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) == HAL_OK, "ADC LINEARITY calibration failed");

        // 启动ADC DMA
        APPKIT_RAISE_IF_NOT(HAL_ADC_Start_DMA(handle_, buffer.GetData<uint32_t>(), channel_count_ * filter_size_) == HAL_OK, "ADC start DMA failed");

        APPKIT_LOG_DEBUG("ADC <%p> init success", handle_->Instance);
    }

    STM32ADC::~STM32ADC()
    {
        if (handle_ != nullptr && handle_->State != HAL_ADC_STATE_RESET)
        {
            HAL_ADC_Stop_DMA(handle_);
        }
    }
} // namespace stm32

#endif