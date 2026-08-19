#include <stm32_rng.hpp>

#ifdef HAL_RNG_MODULE_ENABLED

#include <logger.hpp>

namespace appkit::stm32
{
    STM32RNG::STM32RNG(const char *name, RNG_HandleTypeDef &handle)
        : handle_(&handle)
    {
        APPKIT_RAISE_IF_NOT(nullptr != name, "Device name must not be null");

        // 检查是否重复创建
        APPKIT_RAISE_IF_NOT(nullptr == RamFs::Dev().Find(name), "Device already exists");

        // 启用外设
        __HAL_RNG_ENABLE(handle_);
        __HAL_RNG_ENABLE_IT(handle_);

        // 创建设备文件
        map_ = this;
        APPKIT_RAISE_IF_NOT(Check(Block::RegisterDevice(name)), "Failed to register device");
    }

    size_t STM32RNG::GetRawNum()
    {
        if (!isOld_)
        {
            // 上一个随机数还未被处理，返回上一个随机数
            size_t temp = lastRand_;
            isOld_ = true;
            return temp;
        }

        // 回退使用随机数算法生成随机数
        return xorShift_.Get();
    }

    __RAM_FUNC void STM32RNG::DataReadyCallback(RNG_HandleTypeDef *hrng, uint32_t random32bit)
    {
        auto self = map_;
        if (self == nullptr) [[unlikely]]
        {
            return;
        }

        // 判断随机数是否正常
        if (random32bit == self->lastRand_) [[unlikely]]
        {
            // (芯片手册)连续随机数发生器测试:
            // 产生的第一个随机数不应使用,随后产生的每个随机数都需要与产生的上一个随机数进行比较。如果任何一对进行比较的数字相等，则测试失败
        }
        else
        {
            // 更新随机数
            if (self->isOld_)
            {
                self->lastRand_ = random32bit;
                self->xorShift_.SetSeed(random32bit);
                self->isOld_ = false;
            }
        }

        // 重新启用中断
        __HAL_RNG_ENABLE_IT(self->handle_);
    }
} // namespace appkit::stm32

extern "C"
{
    [[gnu::optimize("O2")]] void HAL_RNG_ReadyDataCallback(RNG_HandleTypeDef *hrng, uint32_t random32bit)
    {
        using namespace appkit::stm32;

        STM32RNG::DataReadyCallback(hrng, random32bit);
    }
}

#endif // HAL_RNG_MODULE_ENABLED