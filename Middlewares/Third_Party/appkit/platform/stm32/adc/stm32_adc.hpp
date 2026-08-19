#pragma once

#include <main.h>

#ifdef HAL_ADC_MODULE_ENABLED

#include <adc.h>

#include <adc.hpp>
#include <common_type.hpp>

namespace appkit::stm32
{
    class STM32ADC final
    {
        ADC_HandleTypeDef *handle_{nullptr}; ///< ADC句柄指针

        float resolution_{}; ///< 分辨率
        float vref_{};       ///< 参考电压，单位V

        size_t channel_count_{}; ///< 通道数量
        size_t filter_size_{};   ///< 滤波大小
        RawData channel_buffer_; ///< 通道数据缓存

        friend class Channel;

    public:
        /**
         * @brief ADC通道封装
         */
        class Channel final : ADC::Block
        {
            STM32ADC &parent_; ///< 父ADC设备
            size_t index_{};   ///< 通道索引
            float scale_{};    ///< 比例系数
            float offset_{};   ///< 偏移量

            friend class STM32ADC;

        public:
            /**
             * @brief 构造函数
             * @param name 设备名称
             * @param parent 所属ADC设备引用
             * @param index 通道索引
             * @param scale 比例系数
             * @param offset 偏移量
             */
            Channel(const char *name, STM32ADC &parent, size_t index, float scale = 1.0f, float offset = 0.0f);

            ~Channel() override = default;

            /**
             * @brief 读取ADC值
             * @param value 存放读取值的引用
             * @return 错误码
             */
            Result<float> Read() override;
        };

        /**
         * @brief 构造函数
         * @param handle ADC句柄指针
         * @param buffer 通道数据缓存
         * @param vref 参考电压，单位V
         * @param filter_size 滤波大小
         */
        STM32ADC(ADC_HandleTypeDef &handle, RawData buffer, float vref, size_t filter_size = 4);

        /**
         * @brief 析构函数
         */
        ~STM32ADC();
    };
} // namespace appkit::stm32

#endif