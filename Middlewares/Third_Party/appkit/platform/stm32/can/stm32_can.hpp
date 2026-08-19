#pragma once

#include <main.h>

#ifdef HAL_CAN_MODULE_ENABLED

#include <can.h>
#include <can.hpp>

#include <lockfree_pool.hpp>

namespace appkit::stm32
{
    enum class CanId
    {
#ifdef CAN1
        STM32_CAN1,
#endif
#ifdef CAN2
        STM32_CAN2,
#endif
        STM32_CAN_NUMBER,
        STM32_CAN_ID_ERROR
    };

    constexpr CanId GetCanId(const CAN_HandleTypeDef *handle);

    class STM32CAN final : public CAN::Block
    {
        static STM32CAN *map_[std::to_underlying(CanId::STM32_CAN_NUMBER)];
        uint32_t fifo_{};

        CAN_HandleTypeDef *handle_;
        CanId id_{CanId::STM32_CAN_ID_ERROR};

        LockFreePool<CAN::ClassicPack> tx_pool_;

        ErrorCode AddMessage(const CAN::ClassicPack &frame) override;

    public:
        explicit STM32CAN(const char *name, CAN_HandleTypeDef &handle, uint32_t tx_pool_size = 8);

        static void RxCpltCallback(CanId id);

        static void TxCpltCallback(CanId id);
    };
} // namespace appkit::stm32

#endif // HAL_CAN_MODULE_ENABLED
