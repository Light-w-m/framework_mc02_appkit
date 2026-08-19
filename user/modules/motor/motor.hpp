#pragma once

#include <common_type.hpp>
#include <algorithm>

namespace control::motor
{
    /**
     * @brief 电机基类
     */
    class Motor
    {
    public:
        /**
         * @brief 电机测量数据结构
         */
        struct Measure final
        {
            float abs_pos{};     ///< 绝对位置，单位：rad
            float single_pos{};  ///< 单圈位置，单位：rad
            float speed{};       ///< 速度，单位：rad/s
            float torque{};      ///< 力矩，单位：Nm
            float temperature{}; ///< 温度，单位：°C
        };

        /**
         * @brief 力矩控制接口
         * @param torque 目标力矩，单位：Nm
         * @return 错误码
         */
        virtual appkit::ErrorCode TorqueControl(float torque);

        /**
         * @brief 获取电机测量数据
         * @return 电机测量数据引用
         */
        virtual const Measure &GetMeasure() const = 0;
    };

    /**
     * @brief 电机控制器模板类
     * @tparam MotorType 电机类型，必须继承自Motor类
     */
    template <typename MotorType>
        requires std::is_base_of_v<Motor, MotorType>
    class Controller
    {
    };
}