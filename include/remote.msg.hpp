// Auto-generated from remote.msg
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
 * @brief 遥控器数据结构体
 */
struct alignas(4) remote_msg
{
	static inline constexpr const uint32_t HASH = 0x744109ebu;

	struct
	{
		float	rocker_l1;			// 左摇杆竖直方向值 映射到[-1, +1]
		float	rocker_l_;			// 左摇杆水平方向值 映射到[-1, +1]
		float	rocker_r1;			// 右摇杆竖直方向值 映射到[-1, +1]
		float	rocker_r_;			// 右摇杆水平方向值 映射到[-1, +1]
		int8_t	switch_d1;			// 一号二挡开关拨码值 上: 1 | 下: -1
		int8_t	switch_d2;			// 二号二挡开关拨码值 上: 1 | 下: -1
		int8_t	switch_d3;			// 三号二挡开关拨码值 上: 1 | 下: -1
		int8_t	switch_d4;			// 四号二挡开关拨码值 上: 1 | 下: -1
		int8_t	switch_t1;			// 一号三挡开关拨码值 上: 1 | 中: 0 | 下: -1
		int8_t	switch_t2;			// 二号三挡开关拨码值 上: 1 | 中: 0 | 下: -1
		int8_t	switch_t3;			// 三号三挡开关拨码值 上: 1 | 中: 0 | 下: -1
		int8_t	switch_t4;			// 四号三挡开关拨码值 上: 1 | 中: 0 | 下: -1
		float	rocker_e1;			// 一号附加摇杆值 映射到[-1, +1]
		float	rocker_e2;			// 二号附加摇杆值 映射到[-1, +1]
	} data;
};	// struct remote_msg

using RemoteCtrlData = remote_msg;

}

#ifdef __cplusplus
}
#endif

// clang-format on
