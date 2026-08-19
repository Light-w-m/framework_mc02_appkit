#pragma once

#include <main.h>

#ifdef HAL_RNG_MODULE_ENABLED

#include <random.hpp>
#include <xor_shift.hpp>

#include <rng.h>

namespace appkit::stm32
{
    class STM32RNG final : public Random::Block
    {
        static inline STM32RNG *map_ = nullptr;

        RNG_HandleTypeDef *handle_;
        math::XorShift xorShift_{};

        std::atomic_bool isOld_{};
        uint32_t lastRand_{};

        size_t GetRawNum() override;

    public:
        /**
         * @brief 构造函数
         * @param name 设备名称
         * @param handle RNG外设句柄
         */
        explicit STM32RNG(const char *name, RNG_HandleTypeDef &handle);

        /**
         * @brief 析构函数
         */
        ~STM32RNG() override = default;

        /**
         * @brief 数据就绪回调函数
         * @param hrng RNG句柄
         * @param random32bit 生成的随机数
         */
        static void DataReadyCallback(RNG_HandleTypeDef *hrng, uint32_t random32bit);
    };
}

#endif // HAL_RNG_MODULE_ENABLED
