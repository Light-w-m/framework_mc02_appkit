// Auto-generated from imu.msg
// Protocol Version: 1.0.0
// DO NOT EDIT
#pragma once

#include <cstddef>
#include <cstdint>

// clang-format off

#ifdef __cplusplus
extern "C"
{
#endif

namespace sensor_data
{

/**
 * @brief 六轴IMU原始数据结构体
 */
struct alignas(4) imu_msg
{
	static inline constexpr const uint32_t HASH = 0xfb7be962u;

	struct
	{
		float	timepoint;			// 时间戳 ms
		float	accel_data[3];			// [x, y, z] 三轴加速度计数据 m/s
		float	gyro_data[3];			// [x, y, z] 三轴角速度计数据 rad/s
	} data;
};	// struct imu_msg

using IMURawData = imu_msg;

}

#ifdef __cplusplus
}
#endif

// clang-format on
