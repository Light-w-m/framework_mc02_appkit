#pragma once

#include <main.h>

#ifdef HAL_FDCAN_MODULE_ENABLED

#include <fdcan.h>
#include <can.hpp>

#include <lockfree_pool.hpp>

namespace appkit::stm32
{
    enum class FDCanId
    {
#ifdef FDCAN1
        STM32_FDCAN1,
#endif
#ifdef FDCAN2
        STM32_FDCAN2,
#endif
#ifdef FDCAN3
        STM32_FDCAN3,
#endif
        STM32_FDCAN_NUMBER,
        STM32_FDCAN_ID_ERROR
    };

    constexpr FDCanId GetFDCanId(const FDCAN_HandleTypeDef *handle);

    class STM32FDCAN final : CAN::Block, FDCAN::Block
    {
        /* clang-format off */
        static constexpr inline uint32_t FDCAN_PACK_LEN_MAP[16] = {
            FDCAN_DLC_BYTES_0, FDCAN_DLC_BYTES_1, FDCAN_DLC_BYTES_2, FDCAN_DLC_BYTES_3,
            FDCAN_DLC_BYTES_4, FDCAN_DLC_BYTES_5, FDCAN_DLC_BYTES_6, FDCAN_DLC_BYTES_7,
            FDCAN_DLC_BYTES_8, FDCAN_DLC_BYTES_12, FDCAN_DLC_BYTES_16, FDCAN_DLC_BYTES_20,
            FDCAN_DLC_BYTES_24, FDCAN_DLC_BYTES_32, FDCAN_DLC_BYTES_48, FDCAN_DLC_BYTES_64,
        };

        static constexpr inline uint32_t FDCAN_PACK_LEN_TO_INT_MAP[16] = {
            0, 1, 2, 3, 4, 5, 6, 7,
            8, 12, 16, 20, 24, 32, 48, 64,
        };
        /* clang-format on */

        static STM32FDCAN *map_[std::to_underlying(FDCanId::STM32_FDCAN_NUMBER)];

        FDCAN_HandleTypeDef *handle_;
        FDCanId id_{FDCanId::STM32_FDCAN_ID_ERROR};

        LockFreePool<FDCAN::FDPack> tx_pool_fd_;
        LockFreePool<CAN::ClassicPack> tx_pool_;

        FDCAN_TxHeaderTypeDef tx_header_;
        FDCAN_RxHeaderTypeDef rx_header_;

        union
        {
            FDCAN::FDPack fd_pack;
            CAN::ClassicPack classic_pack;
        } tx_buffer_, rx_buffer_;

        ErrorCode AddMessage(const FDCAN::FDPack &frame) override;

        ErrorCode AddMessage(const CAN::ClassicPack &frame) override;

    public:
        explicit STM32FDCAN(const char *name, FDCAN_HandleTypeDef &handle, uint32_t tx_pool_size_fd = 8,
                       uint32_t tx_pool_size_can = 8);

        static void RxCallback(FDCanId id, uint32_t fifo);

        static void TxCallback(FDCanId id);
    };

    /**
     * @brief 检查多个FDCAN实例的Message RAM配置是否冲突
     * @tparam T FDCAN_HandleTypeDef类型参数包
     * @param args FDCAN_HandleTypeDef参数包
     * @note 该函数在运行期进行检查，要求所有FDCAN实例的Message RAM配置不重叠且总大小不超过2560 words
     * @note 该函数要求传入的FDCAN_HandleTypeDef实例数量与FDCanId枚举中的实例数量一致
     *
     * @example FDCAN_MsgRAMCheck(hfdcan1, hfdcan2, hfdcan3); // 检查三个FDCAN实例
     */
    template <typename... T>
        requires(sizeof...(T) == std::to_underlying(FDCanId::STM32_FDCAN_NUMBER) && std::conjunction_v<std::is_same<T, FDCAN_HandleTypeDef>...>)
    FORCE_INLINE void FDCAN_MsgRAMCheck(const T &...args)
    {
        /**
         * @brief 计算Message RAM使用大小
         * @param c FDCAN初始化配置
         */
        static auto GetUse = [](const decltype(FDCAN_HandleTypeDef::Init) &c) -> size_t
        {
            /**
             * @brief 计算单个Message RAM元素占用大小（单位：words）
             */
            static const auto FDCAN_ElmtWords = [](size_t size) -> size_t
            {
                // 2 words header + datas
                switch (size)
                {
                case FDCAN_DATA_BYTES_8:
                    return 4;
                case FDCAN_DATA_BYTES_12:
                    return 5;
                case FDCAN_DATA_BYTES_16:
                    return 6;
                case FDCAN_DATA_BYTES_20:
                    return 7;
                case FDCAN_DATA_BYTES_24:
                    return 8;
                case FDCAN_DATA_BYTES_32:
                    return 10;
                case FDCAN_DATA_BYTES_48:
                    return 14;
                case FDCAN_DATA_BYTES_64:
                    return 18;
                default:
                    return 4;
                }
            };

            return c.StdFiltersNbr * 1u + c.ExtFiltersNbr * 2u +
                   c.RxFifo0ElmtsNbr * FDCAN_ElmtWords(c.RxFifo0ElmtSize) +
                   c.RxFifo1ElmtsNbr * FDCAN_ElmtWords(c.RxFifo1ElmtSize) +
                   c.RxBuffersNbr * FDCAN_ElmtWords(c.RxBufferSize) + c.TxEventsNbr * 2u +
                   (c.TxBuffersNbr + c.TxFifoQueueElmtsNbr) * FDCAN_ElmtWords(c.TxElmtSize);
        };

        /**
         * @brief FDCAN Message RAM使用情况
         */
        struct BufferData
        {
            size_t offset; /// Message RAM偏移地址
            size_t size;   /// Message RAM使用大小

            /**
             * @brief 构造函数
             * @param handle FDCAN句柄
             */
            explicit BufferData(const FDCAN_HandleTypeDef &handle) noexcept
                : offset(handle.Init.MessageRAMOffset), size(GetUse(handle.Init)) {}
        };

        // 获取所有Message RAM使用情况
        BufferData buffers[] = {BufferData(args)...};

        // 使用情况数组排序
        std::sort(std::begin(buffers), std::end(buffers), [](const BufferData &a, const BufferData &b) {
            return a.offset < b.offset;
        });

        size_t total_size = 0;
        for (size_t i = 0; i < sizeof...(T); i++)
        {
            total_size += buffers[i].size;

            // 检查是否重叠
            APPKIT_RAISE_IF_NOT(buffers[i].offset + buffers[i].size <= ((sizeof...(T) != i + 1) ? buffers[i + 1].offset : 2560), "FDCAN Message RAM configuration conflict");
        }

        // 检查总大小
        APPKIT_RAISE_IF_NOT(total_size <= 2560, "FDCAN Message RAM total size exceeds limit"); // Message RAM最大2560 words
    }
} // namespace appkit::stm32

#endif