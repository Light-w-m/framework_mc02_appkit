// Auto-generated from vision_ctrl.msg
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
 * @brief 自瞄控制数据结构体
 */
struct alignas(4) vision_ctrl_msg
{
	static inline constexpr const uint32_t HASH = 0x069bbf35u;

	struct
	{
		bool	is_online;			// 在线标志位
		bool	is_tracking;			// 瞄准状态标志位
		float	aim_droll;			// 目标Roll角度偏移量 映射到[-Pi, +Pi]
		float	aim_dpitch;			// 目标Pitch角度偏移量 映射到[-Pi, +Pi]
		float	aim_dyaw;			// 目标Yaw角度偏移量 映射到[-Pi, +Pi]
		float	aim_drift;			// 当前瞄准偏移率 映射到[0, +1]
	} data;
};	// struct vision_ctrl_msg

using VisionCtrlData = vision_ctrl_msg;

}

#ifdef __cplusplus
}
#endif

// clang-format on
