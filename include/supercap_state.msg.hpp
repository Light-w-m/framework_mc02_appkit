// Auto-generated from supercap_state.msg
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
 * @brief 超级电容数据结构体
 */
struct alignas(4) supercap_state_msg
{
	static inline constexpr const uint32_t HASH = 0xbc2420fau;

	struct
	{
		bool	is_online;			// 在线标志位
		uint8_t	error_code;			// 错误枚举值
		float	current_power;			// 当前功率 W
		float	available_energy;			// 剩余容量 映射到[0, +1]
	} data;
};	// struct supercap_state_msg

using SuperCapData = supercap_state_msg;

}

#ifdef __cplusplus
}
#endif

// clang-format on
