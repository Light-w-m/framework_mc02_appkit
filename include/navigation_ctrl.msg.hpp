// Auto-generated from navigation_ctrl.msg
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

namespace control
{

/**
 * @brief 导航控制数据结构体
 */
struct alignas(4) navigation_ctrl_msg
{
	static inline constexpr const uint32_t HASH = 0x5ba11f4eu;

	struct
	{
		bool	is_online;			// 在线标志位
		float	vx;			// X轴线速度 m/s
		float	vy;			// Y轴线速度 m/s
		float	wz;			// Z轴角速度 rad/s
	} data;
};	// struct navigation_ctrl_msg

using NaviCtrlData = navigation_ctrl_msg;

}

#ifdef __cplusplus
}
#endif

// clang-format on
