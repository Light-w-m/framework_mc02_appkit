#pragma once

#include <driver.hpp>
#include <osal_mutex.hpp>

namespace appkit
{
    /**
     * @brief PWM设备
     */
    class PWM final
    {
    public:
        /**
         * @brief 修改状态
         */
        enum class ModifyState : uint8_t
        {
            None = 0,      ///< 无修改
            Frequency = 1, ///< 频率修改
            DutyCycle = 2, ///< 占空比修改
            Running = 3,   ///< 运行状态修改
        };

        /**
         * @brief PWM设备块基类
         */
        class Block : public Driver<Block>
        {
        public:
            osal::Mutex mutex{}; ///< 互斥锁

            float frequency{};  ///< 频率，单位Hz
            float duty_cycle{}; ///< 占空比，范围0.0~1.0
            bool running{};     ///< 是否正在运行

            /**
             * @brief 析构函数
             */
            virtual ~Block() = default;

            /**
             * @brief 应用修改
             * @param modify 修改类型
             * @return 错误码
             * @note 根据修改类型应用对应的修改
             */
            virtual ErrorCode ApplyModify(ModifyState modify) = 0;
        };

    private:
        Block *block_{}; ///< 设备块

    public:
        /**
         * @brief 构造函数
         */
        constexpr PWM() = default;

        /**
         * @brief 打开PWM设备
         * @param name 设备名称
         * @return 错误码
         */
        ErrorCode Open(const char *name);

        /**
         * @brief 启用PWM输出
         * @return 错误码
         */
        [[nodiscard]] ErrorCode Enable() const;

        /**
         * @brief 禁用PWM输出
         * @return 错误码
         */
        [[nodiscard]] ErrorCode Disable() const;

        /**
         * @brief 设置PWM频率
         * @param frequency 频率，单位Hz
         * @return 错误码
         */
        [[nodiscard]] ErrorCode SetFrequency(float frequency) const;

        /**
         * @brief 设置PWM占空比
         * @param duty_cycle 占空比，范围0.0~1.0
         * @return 错误码
         * @note 占空比范围0.0~1.0，超出范围会被限制
         */
        [[nodiscard]] ErrorCode SetDutyCycle(float duty_cycle) const;
    };
}
