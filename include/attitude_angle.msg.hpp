// Auto-generated from attitude_angle.msg
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

struct alignas(4) attitude_angle_msg
{
	static inline constexpr const uint32_t HASH = 0x26887ba6u;

	struct
	{
		float	timepoint;			// 时间戳 ms
		float	roll;			// 欧拉角 Roll rad
		float	pitch;			// 欧拉角 Pitch rad
		float	yaw;			// 欧拉角 Yaw rad
		float	yaw_total;			// 欧拉角 Yaw 累计值 rad
		float	w;			// 四元数 W
		float	x;			// 四元数 X
		float	y;			// 四元数 Y
		float	z;			// 四元数 Z
	} data;
};	// struct attitude_angle_msg

using AttitudeAngle = attitude_angle_msg;

}

#ifdef __cplusplus
}
#endif

// clang-format on
