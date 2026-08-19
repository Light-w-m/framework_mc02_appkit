#include <stm32_can.hpp>

#ifdef HAL_CAN_MODULE_ENABLED

#include <logger.hpp>

namespace appkit::stm32
{
    constexpr CanId GetCanId(const CAN_HandleTypeDef *handle)
    {
        const auto addr = handle->Instance;
        if (addr == nullptr)
            return CanId::STM32_CAN_ID_ERROR;
#ifdef CAN1
        if (addr == CAN1)
            return CanId::STM32_CAN1;
#endif
#ifdef CAN2
        if (addr == CAN2)
            return CanId::STM32_CAN2;
#endif

        return CanId::STM32_CAN_ID_ERROR;
    }

    STM32CAN *STM32CAN::map_[std::to_underlying(CanId::STM32_CAN_NUMBER)] = {nullptr};

    STM32CAN::STM32CAN(const char *name, CAN_HandleTypeDef &handle, const uint32_t tx_pool_size)
        : handle_(&handle), tx_pool_(tx_pool_size)
    {
        // 检查是否重复创建
        APPKIT_RAISE_IF_NOT(nullptr == RamFs::Dev().Find(name), "Device already exists");

        // 初始化CAN接收
        CAN_FilterTypeDef can_filter = {};

        can_filter.FilterIdHigh = 0;
        can_filter.FilterIdLow = 0;
        can_filter.FilterMode = CAN_FILTERMODE_IDMASK;
        can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
        can_filter.FilterMaskIdHigh = 0;
        can_filter.FilterMaskIdLow = 0;
        can_filter.FilterActivation = ENABLE;

#ifdef CAN2
        if (id_ == CanId::STM32_CAN1)
        {
            can_filter.FilterBank = 0;
            can_filter.SlaveStartFilterBank = 14;
            can_filter.FilterFIFOAssignment = fifo_ = CAN_RX_FIFO0;
        }
        else if (id_ == CanId::STM32_CAN2)
        {
            can_filter.FilterBank = 14;
            can_filter.SlaveStartFilterBank = 14;
            can_filter.FilterFIFOAssignment = fifo_ = CAN_RX_FIFO1;
        }
#elifdef CAN1
        if (id_ == CanId::STM32_CAN1)
        {
            can_filter.FilterBank = 0;
            can_filter.SlaveStartFilterBank = 14;
            can_filter.FilterFIFOAssignment = fifo_ = CAN_RX_FIFO0;
        }
#else
#error "bxCAN error defined!"
#endif
        else
        {
            // 未知的CAN ID
            APPKIT_RAISE("Unknown CAN ID");
        }

        // 配置过滤器
        APPKIT_RAISE_IF_NOT(HAL_CAN_ConfigFilter(handle_, &can_filter) == HAL_OK, "CAN filter configuration failed");
        APPKIT_RAISE_IF_NOT(HAL_CAN_Start(handle_) == HAL_OK, "CAN start failed");

        // 启用接收中断
        if (fifo_ == CAN_RX_FIFO0)
        {
            HAL_CAN_ActivateNotification(handle_, CAN_IT_RX_FIFO0_MSG_PENDING);
        }
        else
        {
            HAL_CAN_ActivateNotification(handle_, CAN_IT_RX_FIFO1_MSG_PENDING);
        }

        HAL_CAN_ActivateNotification(handle_, CAN_IT_ERROR | CAN_IT_TX_MAILBOX_EMPTY);

        // 注册设备文件
        map_[std::to_underlying(id_)] = this;
        APPKIT_RAISE_IF_NOT(Check(Block::RegisterDevice(name)), "Failed to register device");
    }

    ErrorCode STM32CAN::AddMessage(const CAN::ClassicPack &frame)
    {
        using PackType = CAN::PackType;

        CAN_TxHeaderTypeDef tx_header = {};
        tx_header.DLC = frame.data_len;
        if (frame.data_len > 8)
        {
            return ErrorCode::InvalidArg;
        }

        switch (frame.type)
        {
        case PackType::Standard:
            tx_header.IDE = CAN_ID_STD;
            tx_header.RTR = CAN_RTR_DATA;
            tx_header.StdId = frame.id & 0x7FFU;
            break;
        case PackType::EXTENDED:
            tx_header.IDE = CAN_ID_EXT;
            tx_header.RTR = CAN_RTR_DATA;
            tx_header.ExtId = frame.id & 0x1FFFFFFFU;
            break;
        case PackType::REMOTE_STANDARD:
            tx_header.IDE = CAN_ID_STD;
            tx_header.RTR = CAN_RTR_REMOTE;
            tx_header.StdId = frame.id & 0x7FFU;
            break;
        case PackType::REMOTE_EXTENDED:
            tx_header.IDE = CAN_ID_EXT;
            tx_header.RTR = CAN_RTR_REMOTE;
            tx_header.ExtId = frame.id & 0x1FFFFFFFU;
            break;
        default:
            APPKIT_LOG_ERROR("Unknown CAN PackType");
            return ErrorCode::Failed;
        }

        if (HAL_CAN_GetTxMailboxesFreeLevel(handle_) == 0)
        {
            // 没有空闲的发送邮箱
            goto unwrap;
        }

        uint32_t tx_mailbox;
        if (HAL_CAN_AddTxMessage(handle_, &tx_header, frame.data, &tx_mailbox) != HAL_OK)
        {
            // 发送失败
            goto unwrap;
        }

        return ErrorCode::OK;

    unwrap:
        // 尝试放入发送池中
        if (!Check(tx_pool_.Put(frame)))
        {
            // 发送池已满
            APPKIT_LOG_ERROR("CAN Tx pool is full, dropping message with ID: %d", frame.id);
            return ErrorCode::Full;
        }

        return ErrorCode::Ok;
    }

    __RAM_FUNC void CAN::RxCpltCallback(CanId id)
    {
        using PackType = CAN::PackType;

        const auto *can = map_[std::to_underlying(id)];
        if (can == nullptr)
        {
            APPKIT_RAISE_FROM_CALLBACK(true, "Invalid CAN ID in RxCpltCallback");
        }

        CAN_RxHeaderTypeDef rx_header = {};
        CAN::ClassicPack frame = {};

        do
        {
            // 从接收FIFO中获取消息
            if (HAL_CAN_GetRxMessage(can->handle_, can->fifo_, &rx_header, frame.data) != HAL_OK)
            {
                // 获取消息失败
                return;
            }

            if (rx_header.IDE == CAN_ID_STD)
            {
                frame.id = rx_header.StdId;
                if (rx_header.RTR == CAN_RTR_DATA)
                {
                    frame.type = PackType::Standard;
                }
                else
                {
                    frame.type = PackType::REMOTE_STANDARD;
                }
            }
            else if (rx_header.IDE == CAN_ID_EXT)
            {
                frame.id = rx_header.ExtId;
                if (rx_header.RTR == CAN_RTR_DATA)
                {
                    frame.type = PackType::EXTENDED;
                }
                else
                {
                    frame.type = PackType::REMOTE_EXTENDED;
                }
            }
            else
            {
                // 未知的ID类型
                APPKIT_RAISE_FROM_CALLBACK(true, "Unknown CAN ID type");
            }

            frame.data_len = rx_header.DLC;

            // 处理接收到的消息
            can->OnMessage(frame, true);
        } while (HAL_CAN_GetRxFifoFillLevel(can->handle_, can->fifo_) > 0);
    }

    __RAM_FUNC void CAN::TxCpltCallback(CanId id)
    {
        using PackType = CAN::PackType;

        auto *can = map_[std::to_underlying(id)];
        if (can == nullptr)
        {
            APPKIT_RAISE_FROM_CALLBACK(true, "Invalid CAN ID in TxCpltCallback");
        }

        // 检测是否有空闲的发送邮箱
        CAN::ClassicPack frame{};
        do
        {
            // 从发送池中获取下一个待发送的消息
            if (Check(can->tx_pool_.Get(frame)))
            {
                CAN_TxHeaderTypeDef tx_header = {};
                tx_header.DLC = frame.data_len;

                if (frame.type == PackType::Standard)
                {
                    tx_header.IDE = CAN_ID_STD;
                    tx_header.RTR = CAN_RTR_DATA;
                    tx_header.StdId = frame.id & 0x7FFU;
                }
                else if (frame.type == PackType::EXTENDED)
                {
                    tx_header.IDE = CAN_ID_EXT;
                    tx_header.RTR = CAN_RTR_DATA;
                    tx_header.ExtId = frame.id & 0x1FFFFFFFU;
                }
                else if (frame.type == PackType::REMOTE_STANDARD)
                {
                    tx_header.IDE = CAN_ID_STD;
                    tx_header.RTR = CAN_RTR_REMOTE;
                    tx_header.StdId = frame.id & 0x7FFU;
                }
                else if (frame.type == PackType::REMOTE_EXTENDED)
                {
                    tx_header.IDE = CAN_ID_EXT;
                    tx_header.RTR = CAN_RTR_REMOTE;
                    tx_header.ExtId = frame.id & 0x1FFFFFFFU;
                }
                else
                {
                    // 无效的包类型，跳过
                    continue;
                }

                // 重新添加到发送队列
                uint32_t tx_mailbox;
                if (HAL_CAN_AddTxMessage(can->handle_, &tx_header, frame.data, &tx_mailbox) != HAL_OK)
                {
                    // 发送失败
                    break;
                }
            }
            else
            {
                // 发送池已空，退出循环
                break;
            }
        } while (HAL_CAN_GetTxMailboxesFreeLevel(can->handle_) > 0);
    }
} // namespace stm32

extern "C"
{
    using namespace appkit::stm32;

    [[gnu::optimize("O2")]] void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
    {
        STM32CAN::RxCpltCallback(GetCanId(hcan));
    }

    [[gnu::optimize("O2")]] void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
    {
        STM32CAN::RxCpltCallback(GetCanId(hcan));
    }

    [[gnu::optimize("O2")]] void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
    {
        HAL_CAN_ResetError(hcan);
    }

    [[gnu::optimize("O2")]] void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
    {
        STM32CAN::TxCpltCallback(GetCanId(hcan));
    }

    [[gnu::optimize("O2")]] void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
    {
        STM32CAN::TxCpltCallback(GetCanId(hcan));
    }

    [[gnu::optimize("O2")]] void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
    {
        STM32CAN::TxCpltCallback(GetCanId(hcan));
    }
} // extern "C"

#endif // HAL_CAN_MODULE_ENABLED
