#include <stm32_spi.hpp>

#ifdef HAL_SPI_MODULE_ENABLED

#include <logger.hpp>

namespace appkit::stm32
{
    constexpr SpiId GetSpiId(const SPI_HandleTypeDef *handle)
    {
        if (handle == nullptr)
            return SpiId::STM32_SPI_ID_ERROR;

        const auto addr = handle->Instance;
        if (addr == nullptr)
            return SpiId::STM32_SPI_ID_ERROR;
#ifdef SPI1
        if (addr == SPI1)
            return SpiId::STM32_SPI1;
#endif
#ifdef SPI2
        if (addr == SPI2)
            return SpiId::STM32_SPI2;
#endif
#ifdef SPI3
        if (addr == SPI3)
            return SpiId::STM32_SPI3;
#endif
#ifdef SPI4
        if (addr == SPI4)
            return SpiId::STM32_SPI4;
#endif
#ifdef SPI5
        if (addr == SPI5)
            return SpiId::STM32_SPI5;
#endif
#ifdef SPI6
        if (addr == SPI6)
            return SpiId::STM32_SPI6;
#endif
#ifdef SPI7
        if (addr == SPI7)
            return SpiId::STM32_SPI7;
#endif
#ifdef SPI8
        if (addr == SPI8)
            return SpiId::STM32_SPI8;
#endif
        return SpiId::STM32_SPI_ID_ERROR;
    }

    STM32SPI *STM32SPI::map_[std::to_underlying(SpiId::STM32_SPI_NUMBER)]{};

    STM32SPI::Subset::Subset(STM32SPI &parent, const char *name, const char *cs_name)
        : parent_(parent)
    {
        // 判断父设备是否使用单一子设备
        if (parent_.use_single_subset_)
        {
            // 判断是否已有设备注册
            APPKIT_RAISE_IF_NOT(nullptr == parent_.current_subset_, "STM32 SPI device can only have one subset.");
            parent_.current_subset_ = this;
        }
        else
        {
            // 判断是否使用片选GPIO
            if (cs_name)
            {
                // 初始化片选GPIO
                if (ErrorCode::OK != cs_gpio_.Open(cs_name))
                {
                    APPKIT_LOG_ERROR("SPI subset %s CS Pin %s open failed.", name, cs_name);
                    APPKIT_RAISE("SPI subset CS Pin open failed.");
                }

                if (ErrorCode::OK != cs_gpio_.Write(true))
                {
                    APPKIT_LOG_ERROR("SPI subset %s CS Pin %s set high failed.", name, cs_name);
                    APPKIT_RAISE("SPI subset CS Pin set high failed.");
                }
            }
        }

        // 注册设备
        APPKIT_RAISE_IF_NOT(Check(RegisterDevice(name)), "SPI subset device registration failed.");
    }

    ErrorCode STM32SPI::Subset::Select()
    {
        if (parent_.busy_.test_and_set()) [[unlikely]]
        {
            return ErrorCode::BUSY;
        }

        if (cs_gpio_)
        {
            const auto ret = cs_gpio_.Write(false);
            if (ret != ErrorCode::OK)
            {
                parent_.busy_.clear();
                return ret;
            }
        }

        parent_.current_subset_ = this;
        return ErrorCode::OK;
    }

    void STM32SPI::Subset::Unselect()
    {
        if (cs_gpio_)
        {
            cs_gpio_.Write(true);
        }

        if (!parent_.use_single_subset_)
        {
            parent_.current_subset_ = nullptr;
        }

        parent_.busy_.clear();
    }

    ErrorCode STM32SPI::Subset::ReadAndWrite(ConstRawData tx_data, RawData rx_data, ReadOperation &operation)
    {
        // 选择设备
        ErrorCode ret = Select();
        if (!Check(ret))
        {
            return ret;
        }

        // 判断是否需要使用dma
        bool use_dma = false;
        const auto need_write = std::max(tx_data.GetSize(), rx_data.GetSize());
        if (need_write >= parent_.min_dma_use_size_ && parent_.tx_dma_support_ && parent_.rx_dma_support_)
        {
            if (parent_.tx_buffer_.GetSize() < need_write)
            {
                APPKIT_LOG_ERROR("DMA TX buffer size is too small.");

                Unselect();
                return ErrorCode::NO_BUFFER;
            }

            if (parent_.rx_buffer_.GetSize() < need_write)
            {
                APPKIT_LOG_ERROR("DMA RX buffer size is too small.");

                Unselect();
                return ErrorCode::NO_BUFFER;
            }

            tx_data.CopyTo(parent_.tx_buffer_);
            tx_data = parent_.tx_buffer_.SubData(0, need_write);
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
            SCB_CleanDCache_by_Addr(const_cast<uint32_t *>(tx_data.GetData<uint32_t>()), need_write);
#endif
            rx_buffer_ = rx_data;
            rx_data = parent_.rx_buffer_.SubData(0, need_write);
            use_dma = true;
        }

        // 执行读操作
        if (use_dma)
        {
            // 存储当前读操作
            read_operation_ = &operation;

            // 使用DMA方式
            if (HAL_OK != HAL_SPI_TransmitReceive_DMA(parent_.handle_, tx_data.GetData<uint8_t>(), rx_data.GetData<uint8_t>(), need_write))
            {
                ret = ErrorCode::FAILED;
            }
            else
            {
                operation.MarkRunning();
                if (operation.GetType() == ReadOperation::Type::BLOCK)
                {
                    ret = operation.Wait();
                }
            }
        }
        else
        {
            // 使用阻塞方式
            if (HAL_OK != HAL_SPI_TransmitReceive(parent_.handle_, tx_data.GetData<uint8_t>(), rx_data.GetData<uint8_t>(), need_write, 20))
            {
                ret = ErrorCode::FAILED;
            }

            // 取消选中
            Unselect();

            operation.UpdateStatus(false, ret);
            if (operation.GetType() == ReadOperation::Type::BLOCK)
            {
                ret = operation.Wait();
            }
        }

        return ret;
    }

    ErrorCode STM32SPI::Subset::Read(RawData data, ReadOperation &operation)
    {
        // 选择设备
        ErrorCode ret = Select();
        if (!Check(ret))
        {
            return ret;
        }

        // 判断是否需要使用dma
        bool use_dma = false;
        const auto need_read = data.GetSize();
        if (need_read >= parent_.min_dma_use_size_ && parent_.rx_dma_support_)
        {
            if (parent_.rx_buffer_.GetSize() < need_read)
            {
                APPKIT_LOG_ERROR("DMA RX buffer size is too small.");
                Unselect();
                return ErrorCode::NO_BUFFER;
            }

            rx_buffer_ = data;
            data = parent_.rx_buffer_.SubData(0, need_read);
            use_dma = true;
        }

        // 执行读操作
        if (use_dma)
        {
            // 存储当前读操作
            read_operation_ = &operation;

            // 使用DMA方式
            if (HAL_OK != HAL_SPI_Receive_DMA(parent_.handle_, data.GetData<uint8_t>(), need_read))
            {
                ret = ErrorCode::FAILED;
            }
            else
            {
                operation.MarkRunning();
                if (operation.GetType() == ReadOperation::Type::BLOCK)
                {
                    ret = operation.Wait();
                }
            }
        }
        else
        {
            // 使用阻塞方式
            if (HAL_OK != HAL_SPI_Receive(parent_.handle_, data.GetData<uint8_t>(), need_read, 20))
            {
                ret = ErrorCode::FAILED;
            }

            // 取消选中
            Unselect();

            operation.UpdateStatus(false, ret);
            if (operation.GetType() == ReadOperation::Type::BLOCK)
            {
                ret = operation.Wait();
            }
        }

        return ret;
    }

    ErrorCode STM32SPI::Subset::Write(ConstRawData data, WriteOperation &operation)
    {
        // 选择设备
        ErrorCode ret = Select();
        if (!Check(ret))
        {
            return ret;
        }

        // 判断是否需要使用dma
        bool use_dma = false;
        const auto need_write = data.GetSize();
        if (need_write >= parent_.min_dma_use_size_ && parent_.tx_dma_support_)
        {
            if (parent_.tx_buffer_.GetSize() < need_write)
            {
                APPKIT_LOG_ERROR("DMA TX buffer size is too small.");
                Unselect();
                return ErrorCode::NO_BUFFER;
            }

            data.CopyTo(parent_.tx_buffer_);
            data = parent_.tx_buffer_.SubData(0, need_write);
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
            SCB_CleanDCache_by_Addr(const_cast<uint32_t *>(data.GetData<uint32_t>()), need_write);
#endif
            use_dma = true;
        }

        // 执行写操作
        if (use_dma)
        {
            // 存储当前写操作
            write_operation_ = &operation;

            // 使用DMA方式
            if (HAL_OK != HAL_SPI_Transmit_DMA(parent_.handle_, data.GetData<uint8_t>(), need_write))
            {
                ret = ErrorCode::FAILED;
            }
            else
            {
                operation.MarkRunning();
                if (operation.GetType() == ReadOperation::Type::BLOCK)
                {
                    ret = operation.Wait();
                }
            }
        }
        else
        {
            // 使用阻塞方式
            if (HAL_OK != HAL_SPI_Transmit(parent_.handle_, data.GetData<uint8_t>(), need_write, 20))
            {
                ret = ErrorCode::FAILED;
            }

            // 取消选中
            Unselect();

            operation.UpdateStatus(false, ret);
            if (operation.GetType() == ReadOperation::Type::BLOCK)
            {
                ret = operation.Wait();
            }
        }

        return ret;
    }

    STM32SPI::STM32SPI(SPI_HandleTypeDef &spi_handle, RawData tx_buffer, RawData rx_buffer, uint32_t min_dma_use_size)
        : handle_(&spi_handle), tx_buffer_(tx_buffer), rx_buffer_(rx_buffer), min_dma_use_size_(min_dma_use_size)
    {
        id_ = GetSpiId(handle_);
        APPKIT_RAISE_IF_NOT(id_ != SpiId::STM32_SPI_ID_ERROR, "Invalid SPI handle.");

        // 检测是否为主机模式
        if (handle_->Init.Mode != SPI_MODE_MASTER)
        {
            APPKIT_LOG_ERROR("SPI %p is not in master mode.", handle_);
            APPKIT_RAISE("SPI is not in master mode.");
        }

        // 判断是否启用硬件片选
        if (SPI_NSS_SOFT != handle_->Init.NSS)
        {
            APPKIT_LOG_DEBUG("SPI %p uses hardware NSS.", handle_);
            use_single_subset_ = true;
        }
        else
        {
            use_single_subset_ = false;
        }

        // 检测dma缓冲区大小是否足够
        if (handle_->Init.Direction != SPI_DIRECTION_2LINES_RXONLY)
        {
            if (handle_->hdmatx == nullptr)
            {
                APPKIT_LOG_DEBUG("SPI %p TX DMA not supported.", spi_handle);
                tx_dma_support_ = false;
            }
            else
            {
                tx_dma_support_ = true;
                APPKIT_RAISE_IF_NOT(tx_buffer_.GetSize() >= min_dma_use_size_, "TX buffer size is too small for DMA.");
            }
        }

        if (handle_->Init.Direction != SPI_DIRECTION_2LINES_TXONLY)
        {
            if (handle_->hdmarx == nullptr)
            {
                APPKIT_LOG_DEBUG("SPI %p RX DMA not supported.", spi_handle);
                rx_dma_support_ = false;
            }
            else
            {
                rx_dma_support_ = true;
                APPKIT_RAISE_IF_NOT(rx_buffer_.GetSize() >= min_dma_use_size_, "RX buffer size is too small for DMA.");
            }
        }

        // 初始化中断
        map_[std::to_underlying(id_)] = this;

        APPKIT_LOG_DEBUG("STM32 SPI base %p initialized", handle_->Instance);
    }

    __RAM_FUNC void STM32SPI::RxCpltHandler(SpiId id)
    {
        auto spi = map_[std::to_underlying(id)];
        auto &subset = *spi->current_subset_;

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
        SCB_InvalidateDCache_by_Addr(spi->rx_buffer_.GetData<void>(), spi->rx_buffer_.GetSize());
#endif

        spi->rx_buffer_.CopyTo(subset.rx_buffer_);
        subset.Unselect();

        if (subset.read_operation_)
        {
            subset.read_operation_->UpdateStatus(true, ErrorCode::OK);
            subset.read_operation_ = nullptr;
        }
    }

    __RAM_FUNC void STM32SPI::TxCpltHandler(SpiId id)
    {
        auto spi = map_[std::to_underlying(id)];
        auto &subset = *spi->current_subset_;

        subset.Unselect();

        if (subset.write_operation_)
        {
            subset.write_operation_->UpdateStatus(true, ErrorCode::OK);
            subset.write_operation_ = nullptr;
        }
    }

    __RAM_FUNC void STM32SPI::ErrorHandler(SpiId id)
    {
        auto spi = map_[std::to_underlying(id)];
        auto &subset = *spi->current_subset_;

        subset.Unselect();

        if (subset.read_operation_)
        {
            subset.read_operation_->UpdateStatus(true, ErrorCode::FAILED);
            subset.read_operation_ = nullptr;
        }

        if (subset.write_operation_)
        {
            subset.write_operation_->UpdateStatus(true, ErrorCode::FAILED);
            subset.write_operation_ = nullptr;
        }
    }
} // namespace appkit::stm32

extern "C"
{
    using namespace appkit::stm32;

    [[gnu::optimize("O2")]] void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
    {
        STM32SPI::TxCpltHandler(GetSpiId(hspi));
    }

    [[gnu::optimize("O2")]] void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
    {
        STM32SPI::RxCpltHandler(GetSpiId(hspi));
    }

    [[gnu::optimize("O2")]] void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
    {
        STM32SPI::RxCpltHandler(GetSpiId(hspi));
    }

    [[gnu::optimize("O2")]] void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
    {
        STM32SPI::ErrorHandler(GetSpiId(hspi));
    }
}

#endif // HAL_SPI_MODULE_ENABLED