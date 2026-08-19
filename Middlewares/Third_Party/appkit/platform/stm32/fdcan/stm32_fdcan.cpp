#include <stm32_fdcan.hpp>

#ifdef HAL_FDCAN_MODULE_ENABLED

#include <string>

namespace appkit::stm32
{
    constexpr FDCanId GetFDCanId(const FDCAN_HandleTypeDef *handle)
    {
        if (nullptr == handle)
        {
            return FDCanId::STM32_FDCAN_ID_ERROR;
        }

        const auto addr = handle->Instance;
#ifdef FDCAN1
        if (addr == FDCAN1)
        {
            return FDCanId::STM32_FDCAN1;
        }
#endif
#ifdef FDCAN2
        if (addr == FDCAN2)
        {
            return FDCanId::STM32_FDCAN2;
        }
#endif
#ifdef FDCAN3
        if (addr == FDCAN3)
        {
            return FDCanId::STM32_FDCAN3;
        }
#endif

        return FDCanId::STM32_FDCAN_ID_ERROR;
    }

    STM32FDCAN *STM32FDCAN::map_[std::to_underlying(FDCanId::STM32_FDCAN_NUMBER)] = {nullptr};

    STM32FDCAN::STM32FDCAN(const char *name, FDCAN_HandleTypeDef &handle, const uint32_t tx_pool_size_fd,
                 const uint32_t tx_pool_size_can)
        : handle_(&handle), tx_pool_fd_(tx_pool_size_fd), tx_pool_(tx_pool_size_can)
    {
        // 创建经典CAN设备名称
        const std::string classic_name = std::string(name) + "_cl";

        // 检查是否重复创建
        APPKIT_RAISE_IF_NOT(nullptr == RamFs::Dev().Find(name) && nullptr == RamFs::Dev().Find(classic_name.data()), "Device already exists");

        id_ = GetFDCanId(handle_);
        APPKIT_RAISE_IF_NOT(id_ != FDCanId::STM32_FDCAN_ID_ERROR, "Invalid FDCAN ID");

        // 检查是否重复创建
        const auto index = std::to_underlying(id_);
        APPKIT_RAISE_IF_NOT(map_[index] == nullptr, "Device already exists");
        map_[index] = this;

        // 初始化过滤器
        FDCAN_FilterTypeDef filter{};
        filter.IdType = FDCAN_STANDARD_ID;
        filter.FilterIndex = 0;
        filter.FilterType = FDCAN_FILTER_MASK;
        filter.FilterID1 = 0;
        filter.FilterID2 = 0;
        filter.RxBufferIndex = 0;
        filter.IsCalibrationMsg = 0;

#ifdef FDCAN3
        if (id_ == FDCanId::STM32_FDCAN1)
        {
            filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
        }
        else if (id_ == FDCanId::STM32_FDCAN2)
        {
            filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
        }
        else if (id_ == FDCanId::STM32_FDCAN3)
        {
            filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
        }
#else
#ifdef FDCAN2
        if (id_ == FDCanId::STM32_FDCAN1)
        {
            filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
        }
        else if (id_ == FDCanId::STM32_FDCAN2)
        {
            filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
        }
#else
        filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
#endif
#endif

        APPKIT_RAISE_IF_NOT(HAL_FDCAN_ConfigFilter(handle_, &filter) == HAL_OK, "FDCAN filter config failed");

        filter.IdType = FDCAN_EXTENDED_ID;

        APPKIT_RAISE_IF_NOT(HAL_FDCAN_ConfigFilter(handle_, &filter) == HAL_OK, "FDCAN filter config failed");

        if (filter.FilterConfig == FDCAN_FILTER_TO_RXFIFO0)
        {
            HAL_FDCAN_ActivateNotification(handle_, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
        }
        else
        {
            HAL_FDCAN_ActivateNotification(handle_, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);
        }

        HAL_FDCAN_ActivateNotification(handle_, FDCAN_IT_TX_FIFO_EMPTY | FDCAN_IT_TX_COMPLETE, 0);
        HAL_FDCAN_ConfigGlobalFilter(handle_, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

        APPKIT_RAISE_IF_NOT(HAL_FDCAN_Start(handle_) == HAL_OK, "FDCAN start failed");

        // 注册经典CAN设备
        APPKIT_RAISE_IF_NOT(Check(CAN::Block::RegisterDevice(classic_name.data())), "Failed to register classic CAN device");

        // 注册FDCAN设备
        if (handle_->Init.FrameFormat != FDCAN_FRAME_CLASSIC)
        {
            APPKIT_RAISE_IF_NOT(Check(FDCAN::Block::RegisterDevice(name)), "Failed to register FDCAN device");
        }
        else
        {
            APPKIT_LOG_WARNING("FDCAN %s work in classic CAN mode, only classic CAN device registered", name);
        }
    }

    ErrorCode STM32FDCAN::AddMessage(const FDCAN::FDPack &frame)
    {
        using PackType = FDCAN::PackType;

        if (frame.data_len > 64)
        {
            return ErrorCode::INVALID_ARG;
        }

        FDCAN_TxHeaderTypeDef tx_header{};

        tx_header.Identifier = frame.id;
        switch (frame.type)
        {
        case PackType::STANDARD:
            if (frame.id > 0x7FF) [[unlikely]]
            {
                return ErrorCode::INVALID_ARG;
            }

            tx_header.IdType = FDCAN_STANDARD_ID;
            tx_header.TxFrameType = FDCAN_DATA_FRAME;
            break;
        case PackType::EXTENDED:
            if (frame.id > 0x1FFFFFFF) [[unlikely]]
            {
                return ErrorCode::INVALID_ARG;
            }

            tx_header.IdType = FDCAN_EXTENDED_ID;
            tx_header.TxFrameType = FDCAN_DATA_FRAME;
            break;
        case PackType::REMOTE_STANDARD:
            if (frame.id > 0x7FF) [[unlikely]]
            {
                return ErrorCode::INVALID_ARG;
            }

            tx_header.IdType = FDCAN_STANDARD_ID;
            tx_header.TxFrameType = FDCAN_REMOTE_FRAME;
            break;
        case PackType::REMOTE_EXTENDED:
            if (frame.id > 0x1FFFFFFF) [[unlikely]]
            {
                return ErrorCode::INVALID_ARG;
            }

            tx_header.IdType = FDCAN_EXTENDED_ID;
            tx_header.TxFrameType = FDCAN_REMOTE_FRAME;
            break;
        default:
            return ErrorCode::INVALID_ARG;
        }

        if (frame.data_len <= 8)
        {
            tx_header.DataLength = FDCAN_PACK_LEN_MAP[frame.data_len];
        }
        else if (frame.data_len <= 24)
        {
            tx_header.DataLength = FDCAN_PACK_LEN_MAP[(frame.data_len - 9) / 4 + 1 + 8];
        }
        else if (frame.data_len < 32)
        {
            tx_header.DataLength = FDCAN_DLC_BYTES_32;
        }
        else if (frame.data_len < 48)
        {
            tx_header.DataLength = FDCAN_DLC_BYTES_48;
        }
        else
        {
            tx_header.DataLength = FDCAN_DLC_BYTES_64;
        }

        tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
        tx_header.BitRateSwitch = FDCAN_BRS_ON;
        tx_header.FDFormat = FDCAN_FD_CAN;
        tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
        tx_header.MessageMarker = 0x00;

        if (HAL_FDCAN_GetTxFifoFreeLevel(handle_) == 0 || HAL_FDCAN_AddMessageToTxFifoQ(handle_, &tx_header, frame.data) != HAL_OK)
        {
            if (tx_pool_fd_.Put(frame) != ErrorCode::OK)
            {
                return ErrorCode::FAILED;
            }
        }

        return ErrorCode::OK;
    }

    ErrorCode STM32FDCAN::AddMessage(const CAN::ClassicPack &frame)
    {
        using PackType = FDCAN::PackType;

        if (frame.data_len > 8)
        {
            return ErrorCode::INVALID_ARG;
        }

        FDCAN_TxHeaderTypeDef tx_header{};

        tx_header.Identifier = frame.id;
        switch (frame.type)
        {
        case PackType::STANDARD:
            if (frame.id > 0x7FF) [[unlikely]]
            {
                return ErrorCode::INVALID_ARG;
            }

            tx_header.IdType = FDCAN_STANDARD_ID;
            tx_header.TxFrameType = FDCAN_DATA_FRAME;
            break;
        case PackType::EXTENDED:
            if (frame.id > 0x1FFFFFFF) [[unlikely]]
            {
                return ErrorCode::INVALID_ARG;
            }

            tx_header.IdType = FDCAN_EXTENDED_ID;
            tx_header.TxFrameType = FDCAN_DATA_FRAME;
            break;
        case PackType::REMOTE_STANDARD:
            if (frame.id > 0x7FF) [[unlikely]]
            {
                return ErrorCode::INVALID_ARG;
            }

            tx_header.IdType = FDCAN_STANDARD_ID;
            tx_header.TxFrameType = FDCAN_REMOTE_FRAME;
            break;
        case PackType::REMOTE_EXTENDED:
            if (frame.id > 0x1FFFFFFF) [[unlikely]]
            {
                return ErrorCode::INVALID_ARG;
            }

            tx_header.IdType = FDCAN_EXTENDED_ID;
            tx_header.TxFrameType = FDCAN_REMOTE_FRAME;
            break;
        default:
            return ErrorCode::INVALID_ARG;
        }

        tx_header.DataLength = FDCAN_PACK_LEN_MAP[frame.data_len];
        tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
        tx_header.BitRateSwitch = FDCAN_BRS_OFF;
        tx_header.FDFormat = FDCAN_CLASSIC_CAN;
        tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
        tx_header.MessageMarker = 0x01;

        if (HAL_FDCAN_GetTxFifoFreeLevel(handle_) == 0 || HAL_FDCAN_AddMessageToTxFifoQ(handle_, &tx_header, frame.data) != HAL_OK)
        {
            if (!Check(tx_pool_.Put(frame)))
            {
                return ErrorCode::FAILED;
            }
        }

        return ErrorCode::OK;
    }

    __RAM_FUNC void STM32FDCAN::RxCallback(FDCanId id, uint32_t fifo)
    {
        using PackType = FDCAN::PackType;

        auto self = map_[std::to_underlying(id)];
        if (self == nullptr)
        {
            return;
        }

        auto &buffer = self->rx_buffer_;

        if (HAL_FDCAN_GetRxMessage(self->handle_, fifo, &self->rx_header_, buffer.fd_pack.data) == HAL_OK)
        {
            if (self->rx_header_.FDFormat == FDCAN_FD_CAN)
            {
                buffer.fd_pack.id = self->rx_header_.Identifier;
                switch (self->rx_header_.IdType)
                {
                case FDCAN_STANDARD_ID:
                    buffer.fd_pack.id = self->rx_header_.Identifier & 0x7FF;

                    if (self->rx_header_.RxFrameType == FDCAN_DATA_FRAME)
                    {
                        buffer.fd_pack.type = PackType::STANDARD;
                    }
                    else
                    {
                        buffer.fd_pack.type = PackType::REMOTE_STANDARD;
                    }
                    break;
                case FDCAN_EXTENDED_ID:
                    buffer.fd_pack.id = self->rx_header_.Identifier & 0x1FFFFFFF;

                    if (self->rx_header_.RxFrameType == FDCAN_DATA_FRAME)
                    {
                        buffer.fd_pack.type = PackType::EXTENDED;
                    }
                    else
                    {
                        buffer.fd_pack.type = PackType::REMOTE_EXTENDED;
                    }
                    break;
                default:
                    APPKIT_RAISE_FROM_CALLBACK(true, "Invalid FDCAN ID type");
                    return;
                }

                for (uint32_t i = 0; i < 16; i++)
                {
                    if (self->rx_header_.DataLength == FDCAN_PACK_LEN_MAP[i])
                    {
                        buffer.fd_pack.data_len = FDCAN_PACK_LEN_TO_INT_MAP[i];
                        break;
                    }
                }

                self->FDCAN::Block::OnMessage(buffer.fd_pack, true);
            }
            else
            {
                buffer.classic_pack.id = self->rx_header_.Identifier;
                switch (self->rx_header_.IdType)
                {
                case FDCAN_STANDARD_ID:
                    buffer.classic_pack.id = self->rx_header_.Identifier & 0x7FF;

                    if (self->rx_header_.RxFrameType == FDCAN_DATA_FRAME)
                    {
                        buffer.classic_pack.type = PackType::STANDARD;
                    }
                    else
                    {
                        buffer.classic_pack.type = PackType::REMOTE_STANDARD;
                    }
                    break;
                case FDCAN_EXTENDED_ID:
                    buffer.classic_pack.id = self->rx_header_.Identifier & 0x1FFFFFFF;

                    if (self->rx_header_.RxFrameType == FDCAN_DATA_FRAME)
                    {
                        buffer.classic_pack.type = PackType::EXTENDED;
                    }
                    else
                    {
                        buffer.classic_pack.type = PackType::REMOTE_EXTENDED;
                    }
                    break;
                default:
                    APPKIT_RAISE_FROM_CALLBACK(true, "Invalid FDCAN ID type");
                    return;
                }

                buffer.classic_pack.data_len = self->rx_header_.DataLength;
                if (buffer.classic_pack.data_len > 8)
                {
                    buffer.classic_pack.data_len = 8;
                }

                self->CAN::Block::OnMessage(buffer.classic_pack, true);
            }
        }
    }

    __RAM_FUNC void STM32FDCAN::TxCallback(FDCanId id)
    {
        using PackType = FDCAN::PackType;

        auto self = map_[std::to_underlying(id)];
        if (self == nullptr)
        {
            return;
        }

        // 获取FDCAN帧
        if (Check(self->tx_pool_fd_.Get(self->tx_buffer_.fd_pack)))
        {
            self->tx_header_.Identifier = self->tx_buffer_.fd_pack.id;
            switch (self->tx_buffer_.fd_pack.type)
            {
            case PackType::STANDARD:
                self->tx_header_.IdType = FDCAN_STANDARD_ID;
                self->tx_header_.TxFrameType = FDCAN_DATA_FRAME;
                break;
            case PackType::REMOTE_STANDARD:
                self->tx_header_.IdType = FDCAN_STANDARD_ID;
                self->tx_header_.TxFrameType = FDCAN_REMOTE_FRAME;
                break;
            case PackType::EXTENDED:
                self->tx_header_.IdType = FDCAN_EXTENDED_ID;
                self->tx_header_.TxFrameType = FDCAN_DATA_FRAME;
                break;
            case PackType::REMOTE_EXTENDED:
                self->tx_header_.IdType = FDCAN_EXTENDED_ID;
                self->tx_header_.TxFrameType = FDCAN_REMOTE_FRAME;
                break;
            default:
                APPKIT_RAISE_FROM_CALLBACK(true, "Invalid FDCAN ID type");
                return;
            }

            self->tx_header_.DataLength = FDCAN_PACK_LEN_MAP[self->tx_buffer_.fd_pack.data_len];

            self->AddMessage(self->tx_buffer_.fd_pack);
        }
    }
} // namespace stm32

extern "C"
{
    using namespace appkit::stm32;

    void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan)
    {
        hfdcan->ErrorCode = HAL_FDCAN_ERROR_NONE;
        STM32FDCAN::TxCallback(GetFDCanId(hfdcan));
    }

    void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan,
                                       uint32_t ErrorStatusITs)
    {
        if ((ErrorStatusITs & FDCAN_IT_BUS_OFF) != RESET)
        {
            FDCAN_ProtocolStatusTypeDef protocol_status = {};
            HAL_FDCAN_GetProtocolStatus(hfdcan, &protocol_status);
            if (protocol_status.BusOff)
            {
                CLEAR_BIT(hfdcan->Instance->CCCR, FDCAN_CCCR_INIT);
            }
        }

        STM32FDCAN::TxCallback(GetFDCanId(hfdcan));
    }

    [[gnu::optimize("O2")]] void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hfdcan,
                                            uint32_t BufferIndexes)
    {
        UNUSED(BufferIndexes);
        STM32FDCAN::TxCallback(GetFDCanId(hfdcan));
    }

    [[gnu::optimize("O2")]] void HAL_FDCAN_TxFifoEmptyCallback(FDCAN_HandleTypeDef *hfdcan)
    {
        STM32FDCAN::TxCallback(GetFDCanId(hfdcan));
    }

    [[gnu::optimize("O2")]] void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
    {
        UNUSED(RxFifo0ITs);
        STM32FDCAN::RxCallback(GetFDCanId(hfdcan), FDCAN_RX_FIFO0);
    }

    [[gnu::optimize("O2")]] void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
    {
        UNUSED(RxFifo1ITs);
        STM32FDCAN::RxCallback(GetFDCanId(hfdcan), FDCAN_RX_FIFO1);
    }
}

#endif