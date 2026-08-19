#pragma once

#include <common_time.hpp>
#include <matrix.hpp>

#include <message.hpp>

namespace sensor::imu
{
    /**
     * @brief IMU数据结构体定义
     * @note 假定加速度和角速度坐标系重合，右手坐标系
     */
    struct Data final
    {
        appkit::TimePoint timestamp{}; ///< 时间戳
        appkit::Duration dt{};         ///< 采样时间间隔，单位：秒

        appkit::math::Vectorf<3> accel{}; ///< 加速度，单位：m/s²
        appkit::math::Vectorf<3> gyro{};  ///< 角速度，单位：rad/s
    };

    /**
     * @brief IMU协变量结构体定义
     */
    struct Convars final
    {
        float accel_scale{1.0f}; ///< 加速度缩放系数
        float gyro_scale{1.0f};  ///< 角速度缩放系数

        appkit::math::Matrixf<3, 3> accel_covariance{}; ///< 加速度协方差矩阵，单位：(m/s²)²
        appkit::math::Matrixf<3, 3> gyro_covariance{};  ///< 角速度协方差矩阵，单位：(rad/s)²
    };

    /**
     * @brief 获取IMU话题域
     * @return IMU话题域引用
     */
    inline appkit::Topic::Domain &GetImuTopicDomain()
    {
        static appkit::Topic::Domain domain{"imu"};
        return domain;
    }
}