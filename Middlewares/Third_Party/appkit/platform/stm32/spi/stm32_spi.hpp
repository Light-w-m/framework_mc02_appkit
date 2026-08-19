#pragma once

#include <main.h>

#ifdef HAL_SPI_MODULE_ENABLED

#include <spi.hpp>
#include <gpio.hpp>

#include <spi.h>

namespace appkit::stm32
{
    enum class SpiId
    {
#ifdef SPI1
        STM32_SPI1,
#endif
#ifdef SPI2
        STM32_SPI2,
#endif
#ifdef SPI3
        STM32_SPI3,
#endif
#ifdef SPI4
        STM32_SPI4,
#endif
#ifdef SPI5
        STM32_SPI5,
#endif
#ifdef SPI6
        STM32_SPI6,
#endif
#ifdef SPI7
        STM32_SPI7,
#endif
#ifdef SPI8
        STM32_SPI8,
#endif
        STM32_SPI_NUMBER,
        STM32_SPI_ID_ERROR
    };

    /**
     * @brief 获取SPI ID
     * @param handle SPI句柄
     * @return SPI ID
     */
    constexpr SpiId GetSpiId(const SPI_HandleTypeDef *handle);

    /**
     * @brief STM32 SPI驱动
     */
    class STM32SPI final
    {
        static STM32SPI *map_[std::to_underlying(SpiId::STM32_SPI_NUMBER)]; ///< SPI对象映射表

    public:
        /**
         * @brief SPI子设备
         */
        class Subset final : public SPI::Block
        {
            STM32SPI &parent_;                   ///< 父SPI设备引用
            GPIO cs_gpio_{}; ///< 片选GPIO

            ReadOperation *read_operation_{};   ///< 当前读操作
            WriteOperation *write_operation_{}; ///< 当前写操作

            RawData rx_buffer_; ///< 实际接收缓冲区

            friend class STM32SPI;

            /**
             * @brief 选中设备
             * @return 错误码
             */
            ErrorCode Select();

            /**
             * @brief 取消选中设备
             */
            void Unselect();

        public:
            /**
             * @brief 构造函数
             * @param parent 父SPI设备引用
             * @param name 设备名称
             * @param cs_name 片选GPIO名称
             */
            Subset(STM32SPI &parent, const char *name, const char *cs_name = nullptr);

            /**
             * @brief 读取数据
             * @param data 读取数据存放位置
             * @param operation 读取操作
             * @return 错误码
             */
            ErrorCode Read(RawData data, ReadOperation &operation) override;

            /**
             * @brief 写入数据
             * @param data 写入数据位置
             * @param operation 写入操作
             * @return 错误码
             */
            ErrorCode Write(ConstRawData data, WriteOperation &operation) override;

            /**
             * @brief 读写数据
             * @param tx_data 写入数据位置
             * @param rx_data 读取数据存放位置
             * @param operation 读写操作
             * @return 错误码
             */
            ErrorCode ReadAndWrite(ConstRawData tx_data, RawData rx_data, ReadOperation &operation) override;
        };

        /**
         * @brief 构造函数
         * @param spi_handle SPI句柄
         * @param tx_buffer 发送缓冲区
         * @param rx_buffer 接收缓冲区
         * @param min_dma_use_size 使用DMA的最小传输字节数
         * @note tx_buffer和rx_buffer必须至少有min_dma_use_size大小
         */
        explicit STM32SPI(SPI_HandleTypeDef &spi_handle, RawData tx_buffer, RawData rx_buffer, uint32_t min_dma_use_size = 16);

        /**
         * @brief 读取完成中断处理函数
         * @param id SPI ID
         */
        static void RxCpltHandler(SpiId id);

        /**
         * @brief 写入完成中断处理函数
         * @param id SPI ID
         */
        static void TxCpltHandler(SpiId id);

        /**
         * @brief 错误中断处理函数
         * @param id SPI ID
         */
        static void ErrorHandler(SpiId id);

    private:
        SPI_HandleTypeDef *handle_{};         ///< SPI句柄
        SpiId id_{SpiId::STM32_SPI_ID_ERROR}; ///< SPI ID

        std::atomic_flag busy_{ATOMIC_FLAG_INIT}; ///< 设备忙标志
        bool use_single_subset_{};                ///< 是否只使用单个子设备
        Subset *current_subset_{};                ///< 当前选中的子设备

        RawData tx_buffer_; ///< 发送缓冲区
        RawData rx_buffer_; ///< 接收缓冲区
        uint32_t min_dma_use_size_{};      ///< 使用DMA的最小传输字节数

        bool tx_dma_support_{}; ///< 是否支持DMA发送
        bool rx_dma_support_{}; ///< 是否支持DMA接收

        friend class Subset;
    };
} // namespace appkit::stm32

#endif // HAL_SPI_MODULE_ENABLED
