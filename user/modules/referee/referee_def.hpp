#pragma once

#include <cstdint>

namespace referee
{
#pragma pack(push, 1)
    /**
     * @brief 裁判系统数据帧头结构体
     */
    struct RefereeHeader final
    {
        static constexpr uint8_t FRAME_SOF = 0xA5; ///< 帧起始标志

        uint8_t sof;          ///< 帧起始标志
        uint16_t data_length; ///< 数据长度
        uint8_t seq;          ///< 包序号
        uint8_t crc8;         ///< CRC8 校验码

        /**
         * @brief 计算头部 CRC8 校验码
         * @param header 头部引用
         * @return CRC8 校验码
         */
        static uint8_t CalCrc8(const RefereeHeader &header);
    };

    /* -------------------- 普通链路 -------------------- */

    /**
     * @brief 0x0001
     */
    struct GameStatus final
    {
        /**
         * @brief 比赛类型枚举定义
         */
        enum class GameType : uint8_t
        {
            RMUC = 1,            ///< RoboMaster 机甲大师超级对抗赛
            RMUL_INDIVIDUAL = 2, ///< RoboMaster 机甲大师高校单项赛
            RMUA = 3,            ///< ICRA RoboMaster 高校人工智能挑战赛
            RMUL_3V3 = 4,        ///< RoboMaster 机甲大师高校联盟赛 3V3 对抗
            RMUL_INFANTRY = 5,   ///< RoboMaster 机甲大师高校联盟赛步兵对抗赛
        };

        /**
         * @brief 比赛阶段枚举定义
         */
        enum class GameStage : uint8_t
        {
            NOT_START = 0,       ///< 未开始比赛
            PREPARE = 1,         ///< 准备阶段
            SELF_INSPECTION = 2, ///< 十五秒裁判系统自检阶段
            COUNT_DOWN = 3,      ///< 五秒倒计时
            BATTLE = 4,          ///< 比赛中
            END = 5,             ///< 比赛结算中
        };

        GameType game_type : 4;     ///< 比赛类型
        GameStage game_process : 4; ///< 比赛阶段
        uint16_t stage_remain_time; ///< 当前阶段剩余时间，单位：秒
        uint64_t sync_time_stamp;   ///< UNIX 时间戳，单位：秒
    };

    /**
     * @brief 0x0002
     */
    struct GameResult final
    {
        /**
         * @brief 比赛结果类型枚举定义
         */
        enum class GameResultType : uint8_t
        {
            DOGFALL = 0, ///< 平局
            RED_WIN = 1, ///< 红方获胜
            BLUE_WIN = 2 ///< 蓝方获胜
        };

        GameResultType result; ///< 比赛结果
    };

    /**
     * @brief 0x0003
     */
    struct GameRobotHP final
    {
        uint16_t ally_1_robot_HP; ///< 己方 1 号英雄机器人血量，若该机器人未上场或者被罚下，则血量为 0，下文同理
        uint16_t ally_2_robot_HP; ///< 己方 2 号工程机器人血量
        uint16_t ally_3_robot_HP; ///< 己方 3 号步兵机器人血量
        uint16_t ally_4_robot_HP; ///< 己方 4 号步兵机器人血量
        uint16_t : 16;            ///< 保留位
        uint16_t ally_7_robot_HP; ///< 己方 7 号哨兵机器人血量
        uint16_t ally_outpost_HP; ///< 己方前哨站血量
        uint16_t ally_base_HP;    ///< 己方基地血量
    };

    /**
     * @brief 0x0101
     */
    struct EventData final
    {
        /**
         * @brief 能量机关状态枚举定义
         */
        enum class EnergyMachineStatus : uint8_t
        {
            NOT_ACTIVATED = 0, ///< 能量机关未激活
            ACTIVATED = 1,     ///< 能量机关已激活
            ACTIVATING = 2     ///< 能量机关激活中
        };

        /**
         * @brief 占领状态枚举定义
         */
        enum class OccupyStatus : uint8_t
        {
            NOT_OCCUPIED = 0,   ///< 未被占领
            ALLY_OCCUPIED = 1,  ///< 被己方占领
            ENEMY_OCCUPIED = 2, ///< 被敌方占领
            DUAL_OCCUPIED = 3   ///< 被双方占领
        };

        /**
         * @brief 飞镖目标枚举定义
         */
        enum class DartTarget : uint8_t
        {
            NONE = 0,               ///< 无目标
            OUTPOST = 1,            ///< 击中前哨站
            BASE_FIXED = 2,         ///< 击中基地固定目标
            BASE_RANDOM_FIXED = 3,  ///< 击中基地随机固定目标
            BASE_RANDOM_MOVING = 4, ///< 击中基地随机移动目标
            BASE_END_MOVING = 5     ///< 击中基地末端移动目标
        };

        bool ally_occupy_non_resource_supply_zone : 1;   ///< 己方与资源区区不重叠的补给区占领状态
        bool ally_occupy_above_resource_supply_zone : 1; ///< 己方与资源区重叠的补给区占领状态
        bool ally_occupy_supply_zone : 1;                ///< 己方补给区占领状态（仅 RMUL 适用）

        EnergyMachineStatus ally_small_energy_machine_status : 2; ///< 己方小能量机关状态
        EnergyMachineStatus ally_large_energy_machine_status : 2; ///< 己方大能量机关状态

        OccupyStatus ally_center_highland_occupy_status : 2; ///< 己方中心高地占领状态
        uint8_t ally_trapezoidal_highland_occupy_status : 2; ///< 己方梯形高地占领状态

        uint16_t enemy_dart_last_hit_outpost_base_time : 9; ///< 对方飞镖最后一次击中己方前哨站或基地的时间（0-420，开局默认为 0）
        DartTarget enemy_dart_last_hit_target : 3;          ///< 对方飞镖最后一次击中己方前哨站或基地的具体目标，开局默认为 0，
                                                            ///< 1 为击中前哨站，2 为击中基地固定目标，3 为击中基地随机固定目标，4 为击中基地随机移动目标，5 为击中基地末端移动目标

        OccupyStatus center_gain_point_occupy_status : 2;        ///< 中心增益点的占领状态，0 为未被占领，1 为被己方占领，2 为被对方占领，3 为被双方占领。（仅 RMUL 适用）
        OccupyStatus ally_fortress_gain_point_occupy_status : 2; ///< 己方堡垒增益点的占领状态，0 为未被占领，1 为被己方占领，2 为被对方占领，3 为被双方占领
        OccupyStatus ally_outpost_gain_point_occupy_status : 2;  ///< 己方前哨站增益点的占领状态，0 为未被占领，1 为被己方占领，2 为被对方占领
        bool ally_base_gain_point_occupy_status : 1;             ///< 己方基地增益点的占领状态，0 为未被占领，1 为被己方占领

        uint8_t : 2; ///< 保留位
    };

    /**
     * @brief 0x0104
     */
    struct RefereeWarning final
    {
        /**
         * @brief 判罚等级枚举定义
         */
        enum class PenaltyLevel : uint8_t
        {
            DUAL_YELLOW_CARD = 1, ///< 双方黄牌
            YELLOW_CARD = 2,      ///< 黄牌
            RED_CARD = 3,         ///< 红牌
            LOSS_OF_MATCH = 4     ///< 直接判负
        };

        PenaltyLevel level;         ///< 己方最后一次受到判罚的等级中心增益点
        uint8_t offending_robot_id; ///< 己方最后一次受到判罚的违规机器人 ID。（如红 1 机器人 ID 为 1，蓝1 机器人 ID 为 101）
                                    ///< 判负和双方黄牌时，该值为 0
        uint8_t count;              ///< 己方最后一次受到判罚的违规机器人对应判罚等级的违规次数。（开局默认为 0。）
    };

    /**
     * @brief 0x0105
     */
    struct DartInfo final
    {
        /**
         * @brief 飞镖目标枚举定义
         */
        using DartTarget = EventData::DartTarget;

        uint8_t dart_remaining_time;                 ///< 己方飞镖发射剩余时间，单位：秒
        DartTarget last_dart_target : 3;             ///< 最近一次己方飞镖击中的目标，开局默认为 0，
                                                     ///< 1 为击中前哨站，2 为击中基地固定目标，3 为击中基地随机固定目标，4 为击中基地随机移动目标，5 为击中基地末端移动目标
        uint8_t last_hit_target_cumulative_time : 3; ///< 对方最近被击中的目标累计被击中计次数，开局默认为 0，至多为 4
        DartTarget current_dart_target : 3;          ///< 飞镖此时选定的击打目标，开局默认或未选定/选定前哨站时为 0，
                                                     ///< 选中基地固定目标为 1，选中基地随机固定目标为 2，选中基地随机移动目标为3，选中基地末端移动目标为 4
        uint8_t : 8;
    };

    /**
     * @brief 0x0201
     */
    struct RobotStatus final
    {
        uint8_t robot_id;                         ///< 本机器人 ID
        uint8_t robot_level;                      ///< 机器人等级
        uint16_t current_HP;                      ///< 机器人当前血量
        uint16_t maximum_HP;                      ///< 机器人血量上限
        uint16_t shooter_barrel_cooling_value;    ///< 机器人射击热量每秒冷却值
        uint16_t shooter_barrel_heat_limit;       ///< 机器人射击热量上限
        uint16_t chassis_power_limit;             ///< 机器人底盘功率上限
        bool power_management_gimbal_output : 1;  ///< gimbal 口输出
        bool power_management_chassis_output : 1; ///< chassis 口输出
        bool power_management_shooter_output : 1; ///< shooter 口输出
    };

    /**
     * @brief 0x0202
     */
    struct PowerHeatData final
    {
        uint32_t : 32;                       ///< 保留位
        uint16_t buffer_energy;              ///< 缓冲能量（单位：J）
        uint16_t shooter_17mm_1_barrel_heat; ///< 第 1 个 17mm 发射机构的射击热量
        uint16_t shooter_42mm_barrel_heat;   ///< 42mm 发射机构的射击热量
    };

    /**
     * @brief 0x0203
     */
    struct RobotPos final
    {
        float x;     ///< 本机器人位置 x 坐标，单位：m
        float y;     ///< 本机器人位置 y 坐标，单位：m
        float angle; ///< 本机器人测速模块的朝向，单位：度。正北为 0 度
    };

    /**
     * @brief 0x0204
     */
    struct Buff final
    {
        uint8_t recovery_buff;        ///< 机器人回血增益（百分比，值为 10 表示每秒恢复血量上限的 10%）
        uint16_t cooling_buff;        ///< 机器人射击热量冷却增益具体值（直接值，值为 x 表示热量冷却增加 x/s）
        uint8_t defence_buff;         ///< 机器人防御增益（百分比，值为 50 表示 50%防御增益）
        uint8_t vulnerability_buff;   ///< 机器人负防御增益（百分比，值为 30 表示-30%防御增益）
        uint16_t attack_buff;         ///< 机器人攻击增益（百分比，值为 50 表示 50%攻击增益）
        uint8_t remaining_energy : 6; ///< 机器人剩余能量值反馈，以 16 进制标识机器人剩余能量值比例，仅在机器人剩余能量小于 50%时反馈，其余默认反馈 0x80。机器人初始能量视为 100%
                                      ///< bit 0：在剩余能量≥125%时为 1，其余情况为 0
                                      ///< bit 1：在剩余能量≥100%时为 1，其余情况为 0
                                      ///< bit 2：在剩余能量≥50%时为 1，其余情况为 0
                                      ///< bit 3：在剩余能量≥30%时为 1，其余情况为 0
                                      ///< bit 4：在剩余能量≥15%时为 1，其余情况为 0
                                      ///< bit 5：在剩余能量≥5%时为 1，其余情况为 0
                                      ///< bit 6：在剩余能量≥1%时为 1，其余情况为 0
        uint8_t : 2;                  ///< 保留位
    };

    /**
     * @brief 0x0206
     * @note 0x0206 的受伤害情况为机器人裁判系统本地判定，即时发送，但实际是否受到对应伤害受规则条例影响，请以服务器最终判定为准
     */
    struct HurtData final
    {
        /**
         * @brief 受伤类型枚举定义
         */
        enum class HurtType : uint8_t
        {
            ASSAULTED = 0, ///< 装甲模块被弹丸攻击导致扣血
            OFFLINE = 1,   ///< 装甲模块或超级电容管理模块离线导致扣血
            STRIKE = 5     ///< 装甲模块受到撞击导致扣血
        };

        uint8_t armor_id : 4;
        HurtType HP_deduction_reason : 4;
    };

    /**
     * @brief 0x0207
     */
    struct ShootData final
    {
        /**
         * @brief 弹丸类型枚举定义
         */
        enum class BulletType : uint8_t
        {
            BULLET_17MM = 1, ///< 17mm 弹丸
            BULLET_42MM = 2  ///< 42mm 弹丸
        };

        /**
         * @brief 发射机构枚举定义
         */
        enum class ShootingSource : uint8_t
        {
            SHOOTER_17 = 1, ///< 17mm 发射机构
            SHOOTER_42 = 3  ///< 42mm 发射机构
        };

        BulletType bullet_type;        ///< 弹丸类型
        ShootingSource shooter_number; ///< 发射机构 ID
        uint8_t launching_frequency;   ///< 弹丸射速（单位：Hz）
        float initial_speed;           ///< 弹丸初速度（单位：m/s）
    };

    /**
     * @brief 0x0208
     */
    struct ProjectileAllowance final
    {
        uint16_t projectile_allowance_17mm;     ///< 机器人自身拥有的 17mm 弹丸允许发弹量
        uint16_t projectile_allowance_42mm;     ///< 42mm 弹丸允许发弹量
        uint16_t remaining_gold_coin;           ///< 剩余金币数量
        uint16_t projectile_allowance_fortress; ///< 堡垒增益点提供的储备 17mm 弹丸允许发弹量；该值与机器人是否实际占领堡垒无关
    };

    /**
     * @brief 0x0209
     * @note bit 位值为 1/0 的含义：是否已检测到该增益点 RFID 卡
     * @note 所有 RFID 卡仅在赛内生效。在赛外，即使检测到对应的 RFID 卡，对应值也为 0。
     */
    struct RfidStatus final
    {
        bool ally_base_gain_point : 1;                                                   ///< 己方基地增益点
        bool ally_center_highland_gain_point : 1;                                        ///< 己方中央高地增益点
        bool enemy_center_highland_gain_point : 1;                                       ///< 对方中央高地增益点
        bool ally_trapezoidal_highland_gain_point : 1;                                   ///< 己方梯形高地增益点
        bool enemy_trapezoidal_highland_gain_point : 1;                                  ///< 对方梯形高地增益点
        bool ally_terrain_cross_gain_point_front : 1;                                    ///< 己方地形跨越增益点（飞坡）（靠近己方一侧飞坡前）
        bool ally_terrain_cross_gain_point_after : 1;                                    ///< 己方地形跨越增益点（飞坡）（靠近己方一侧飞坡后）
        bool enemy_terrain_cross_gain_point_front : 1;                                   ///< 对方地形跨越增益点（飞坡）（靠近对方一侧飞坡前）
        bool enemy_terrain_cross_gain_point_after : 1;                                   ///< 对方地形跨越增益点（飞坡）（靠近对方一侧飞坡后）
        bool ally_terrain_cross_gain_point_below_center_highland : 1;                    ///< 己方地形跨越增益点（中央高地下方）
        bool ally_terrain_cross_gain_point_above_center_highland : 1;                    ///< 己方地形跨越增益点（中央高地上方）
        bool enemy_terrain_cross_gain_point_below_center_highland : 1;                   ///< 对方地形跨越增益点（中央高地下方）
        bool enemy_terrain_cross_gain_point_above_center_highland : 1;                   ///< 对方地形跨越增益点（中央高地上方）
        bool ally_terrain_cross_gain_point_below_highway : 1;                            ///< 己方地形跨越增益点（公路下方）
        bool ally_terrain_cross_gain_point_above_highway : 1;                            ///< 己方地形跨越增益点（公路上方）
        bool enemy_terrain_cross_gain_point_below_highway : 1;                           ///< 对方地形跨越增益点（公路下方）
        bool enemy_terrain_cross_gain_point_above_highway : 1;                           ///< 对方地形跨越增益点（公路上方）
        bool ally_fortress_gain_point : 1;                                               ///< 己方堡垒增益点
        bool ally_outpost_gain_point : 1;                                                ///< 己方前哨站增益点
        bool ally_non_resource_supply_zone : 1;                                          ///< 己方与资源区不重叠的补给区/RMUL 补给区
        bool ally_above_resource_supply_zone : 1;                                        ///< 己方与资源区重叠的补给区
        bool ally_assembly_gain_point : 1;                                               ///< 己方装配增益点
        bool enemy_assembly_gain_point : 1;                                              ///< 对方装配增益点
        bool center_gain_point : 1;                                                      ///< 中心增益点（仅 RMUL 适用）
        bool enemy_fortress_gain_point : 1;                                              ///< 对方堡垒增益点
        bool enemy_outpost_gain_point : 1;                                               ///< 对方前哨站增益点
        bool ally_terrain_cross_gain_point_tunnel_near_bottom_of_highway : 1;            ///< 己方地形跨越增益点（隧道）（靠近己方一侧公路区下方）
        bool ally_terrain_cross_gain_point_tunnel_near_top_of_highway : 1;               ///< 己方地形跨越增益点（隧道）（靠近己方一侧公路区上方）
        bool ally_terrain_cross_gain_point_tunnel_near_lower_trapezoidal_highland : 1;   ///< 己方地形跨越增益点（隧道）（靠近己方梯形高地较低处）
        bool ally_terrain_cross_gain_point_tunnel_near_higher_trapezoidal_highland : 1;  ///< 己方地形跨越增益点（隧道）（靠近己方梯形高地较高处）
        bool enemy_terrain_cross_gain_point_tunnel_near_bottom_of_highway : 1;           ///< 对方地形跨越增益点（隧道）（靠近对方一侧公路区下方）
        bool enemy_terrain_cross_gain_point_tunnel_near_top_of_highway : 1;              ///< 对方地形跨越增益点（隧道）（靠近对方一侧公路区上方）
        bool enemy_terrain_cross_gain_point_tunnel_near_lower_trapezoidal_highland : 1;  ///< 对方地形跨越增益点（隧道）（靠近对方梯形高地较低处）
        bool enemy_terrain_cross_gain_point_tunnel_near_higher_trapezoidal_highland : 1; ///< 对方地形跨越增益点（隧道）（靠近对方梯形高地较高处）

        uint8_t : 6; ///< 保留位
    };

    /**
     * @brief 0x020A
     */
    struct DartClientCmd final
    {
        /**
         * @brief 飞镖发射站状态枚举定义
         */
        enum class StationStatus : uint8_t
        {
            OPENED = 0,          ///< 已开启
            CLOSED = 1,          ///< 关闭
            OPENNING_CLOSING = 2 ///< 开启中/关闭中
        };

        StationStatus dart_launch_opening_status; ///< 当前飞镖发射站的状态
        uint8_t : 8;
        uint16_t target_change_time;     ///< 切换击打目标时的比赛剩余时间，单位：秒，无/未切换动作，默认为 0。
        uint16_t latest_launch_cmd_time; ///< 最后一次操作手确定发射指令时的比赛剩余时间，单位：秒，初始值为 0。
    };

    /**
     * @brief 0x020B
     */
    struct GroundRobotPosition final
    {
        float hero_x;       ///< 己方英雄机器人位置 x 轴坐标，单位：m
        float hero_y;       ///< 己方英雄机器人位置 y 轴坐标，单位：m
        float engineer_x;   ///< 己方工程机器人位置 x 轴坐标，单位：m
        float engineer_y;   ///< 己方工程机器人位置 y 轴坐标，单位：m
        float standard_3_x; ///< 己方 3 号步兵机器人位置 x 轴坐标，单位：m
        float standard_3_y; ///< 己方 3 号步兵机器人位置 y 轴坐标，单位：m
        float standard_4_x; ///< 己方 4 号步兵机器人位置 x 轴坐标，单位：m
        float standard_4_y; ///< 己方 4 号步兵机器人位置 y 轴坐标，单位：m

        uint64_t : 64;
    };

    /**
     * @brief 0x020C
     */
    struct RadarMarkData final
    {
        bool enemy_hero_vulnerable : 1;        ///< 对方 1 号英雄机器人易伤情况
        bool enemy_engineer_vulnerable : 1;    ///< 对方 2 号工程机器人易伤情况
        bool enemy_infactory_3_vulnerable : 1; ///< 对方 3 号步兵机器人易伤情况
        bool enemy_infactory_4_vulnerable : 1; ///< 对方 4 号步兵机器人易伤情况
        bool enemy_sentry_vulnerable : 1;      ///< 对方哨兵机器人易伤情况

        bool ally_hero_special_marking : 1;        ///< 己方 1 号英雄机器人特殊标识情况
        bool ally_engineer_special_marking : 1;    ///< 己方 2 号工程机器人特殊标识情况
        bool ally_infactory_3_special_marking : 1; ///< 己方 3 号步兵机器人特殊标识情况
        bool ally_infactory_4_special_marking : 1; ///< 己方 4 号步兵机器人特殊标识情况
        bool ally_sentry_special_marking : 1;      ///< 己方哨兵机器人特殊标识情况

        uint8_t : 6; ///< 保留位
    };

    /**
     * @brief 0x020D
     */
    struct SentryInfo final
    {
        /**
         * @brief 哨兵机器人姿态枚举定义
         */
        enum class SentryPosture : uint8_t
        {
            OFFENSIVE = 1, ///< 进攻姿态
            DEFENSIVE = 2, ///< 防御姿态
            MOBILE = 3     ///< 移动姿态
        };

        uint16_t projectile_allowance : 11;                       ///< 除远程兑换外，哨兵机器人成功兑换的允许发弹量，开局为0，
                                                                  ///< 在哨兵机器人成功兑换一定允许发弹量后，该值将变为哨兵机器人成功兑换的允许发弹量值
        uint8_t remote_exchange_projectile_allowance_count : 4;   ///< 哨兵机器人成功远程兑换允许发弹量的次数，开局为 0，
                                                                  ///< 在哨兵机器人成功远程兑换允许发弹量后，该值将变为哨兵机器人成功远程兑换允许发弹量的次数
        uint8_t remote_exchange_blood_count : 4;                  ///< 哨兵机器人成功远程兑换血量的次数，开局为 0，
                                                                  ///< 在哨兵机器人成功远程兑换血量后，该值将变为哨兵机器人成功远程兑换血量的次数
        bool free_resurrection_flag : 1;                          ///< 哨兵机器人当前是否可以确认免费复活
        bool exchange_immediate_resurrection_flag : 1;            ///< 哨兵机器人当前是否可以兑换立即复活
        uint16_t exchange_immediate_resurrection_cions_cost : 10; ///< 哨兵机器人当前若兑换立即复活需要花费的金币数
        uint32_t : 13;
        SentryPosture current_posture;                  ///< 哨兵当前姿态
        bool ally_energy_machine_activation_rights : 1; ///< 己方能量机关是否能够进入正在激活状态
        uint8_t : 1;
    };

    /**
     * @brief 0x020E
     */
    struct RadarInfo final
    {
        uint8_t trigger_double_vulnerable_count : 2; ///< 雷达是否拥有触发双倍易伤的机会，开局为 0，数值为雷达拥有触发双倍易伤的机会，至多为 2
        bool enemy_triggering_double_vulnerable : 1; ///< 对方是否正在被触发双倍易伤
        uint8_t ally_encryption_level : 2;           ///< 己方加密等级（即对方干扰波难度等级），开局为 1，最高为 3
        bool modify_key_flag : 1;                    ///< 当前是否可以修改密钥
        uint8_t : 2;                                 ///< 保留位
    };

    /**
     * @brief 0x0301
     */
    template <std::size_t N>
        requires(N <= 112)
    struct RobotInteractionData final
    {
        uint16_t data_cmd_id; ///< 子内容 ID需为开放的子内容 ID
        uint16_t sender_id;   ///< 发送者 ID需与自身 ID 匹配，ID 编号详见附录
        uint16_t receiver_id; ///< 接收者 ID 仅限己方通信 | 需为规则允许的多机通讯接收者 | 若接收者为选手端，则仅可发送至发送者对应的选手端 | ID 编号详见附录
        uint8_t user_data[N]; ///< 内容数据段 最大为 112 字节
    };

    /**
     * @brief 0x0301 | 0x0100
     */
    struct InteractionLayerDelete final
    {
        /**
         * @brief 删除类型枚举定义
         */
        enum class DeleteType : uint8_t
        {
            NONE = 0,         ///< 空操作
            DELETE_LAYER = 1, ///< 删除图层
            DELETE_ALL = 2    ///< 删除所有
        };

        DeleteType delete_type; ///< 删除操作
        uint8_t layer;          ///< 图层数
    };

    /**
     * @brief 0x0301 | 0x0101
     */
    struct InteractionFigure final
    {
        /**
         * @brief 操作类型枚举定义
         */
        enum class OperateType : uint8_t
        {
            NONE = 0,   ///< 空操作
            ADD = 1,    ///< 添加图形
            MODIFY = 2, ///< 修改图形
            DELETE = 3  ///< 删除图形
        };

        /**
         * @brief 图形类型枚举定义
         */
        enum class FigureType : uint8_t
        {
            LINE = 0,      ///< 线段
            RECTANGLE = 1, ///< 矩形
            CIRCLE = 2,    ///< 圆形
            ELLIPSE = 3,   ///< 椭圆
            ARC = 4,       ///< 圆弧
            FLOATING = 5,  ///< 浮点数
            INTEGER = 6    ///< 整数
        };

        /**
         * @brief 颜色枚举定义
         */
        enum class Color : uint8_t
        {
            MAIN = 0,     ///< 主色调（红色/蓝色）
            YELLOW = 1,   ///< 黄色
            GREEN = 2,    ///< 绿色
            ORANGE = 3,   ///< 橙色
            AMARANTH = 4, ///< 紫红色
            PINK = 5,     ///< 粉色
            CYAN = 6,     ///< 青色
            BLACK = 7,    ///< 黑色
            WHITE = 8     ///< 白色
        };

        uint8_t figure_name[3];       ///< 图形名 在图形删除、修改等操作中，作为索引
        OperateType operate_type : 3; ///< 图形操作
        FigureType figure_type : 3;   ///< 图形类型
        uint32_t layer : 4;           ///< 图层数（0~9）
        Color color : 4;              ///< 颜色
        uint32_t details_a : 9;
        uint32_t details_b : 9;
        uint32_t width : 10;   ///< 线宽，建议字体大小与线宽比例为 10：1
        uint32_t start_x : 11; ///< 起点/圆心 x 坐标
        uint32_t start_y : 11; ///< 起点/圆心 y 坐标
        uint32_t details_c : 10;
        uint32_t details_d : 11;
        uint32_t details_e : 11;
    };

    /**
     * @brief 0x0301 | 0x0102
     */
    struct InteractionFigure2 final
    {
        InteractionFigure figure[2];
    };

    /**
     * @brief 0x0301 | 0x0103
     */
    struct InteractionFigure3 final
    {
        InteractionFigure figure[5];
    };

    /**
     * @brief 0x0301 | 0x0104
     */
    struct InteractionFigure4 final
    {
        InteractionFigure figure[7];
    };

    /**
     * @brief 0x0301 | 0x0110
     */
    struct ExtClientCustomCharacter final
    {
        InteractionFigure figure_config;
        uint8_t data[30];
    };

    /**
     * @brief 0x0301 | 0x0120
     * @note 在哨兵发送该子命令时，服务器将按照从相对低位到相对高位的原则依次处理这些指令，直至全部成功或不能处理为止
     * @example 若队伍金币数为 0，此时哨兵战亡，“是否确认复活”的值为 1，“是否确认兑换立即复活”的值为 1，“确认兑换的允许发弹量值”为 100。
     * （假定之前哨兵未兑换过允许发弹量）由于此时队伍金币数不足以使哨兵兑换立即复活，则服务器将会忽视后续指令，等待哨兵发送的下一组指令。
     */
    struct SentryCmd final
    {
        /**
         * @brief 哨兵机器人姿态枚举定义
         */
        using SentryPosture = SentryInfo::SentryPosture;

        bool confirm_resurrection : 1;            ///< 哨兵机器人是否确认复活
                                                  ///< false 表示哨兵机器人确认不复活，即使此时哨兵的复活读条已经完成
                                                  ///< true 表示哨兵机器人确认复活，若复活读条完成将立即复活
        bool exchange_immediate_resurrection : 1; ///< 哨兵机器人是否兑换立即复活
                                                  ///< false 表示哨兵机器人确认不兑换立即复活
                                                  ///< true 表示哨兵机器人确认兑换立即复活，若此时哨兵机器人符合兑换立即复活的规则要求，则会立即消耗金币兑换立即复活

        uint16_t exchange_projectile_allowance : 11;            ///< 兵将要兑换的发弹量值，开局为 0，修改此值后，哨兵在补血点即可兑换允许发弹量
                                                                ///< 此值的变化需要单调递增，否则视为不合法。
                                                                ///< 示例：此值开局仅能为 0，此后哨兵可将其从 0 修改至 X，则消耗 X 金币成功兑换 X 允许发弹量。此后哨兵可将其从 X 修改至X+Y，以此类推
        uint8_t remote_exchange_projectile_allowance_count : 4; ///< 哨兵远程兑换发弹量的请求次数，开局为 0，修改此值即可请求远程兑换发弹量
                                                                ///< 此值的变化需要单调递增且每次仅能增加 1，否则视为不合法。
                                                                ///< 示例：此值开局仅能为 0，此后哨兵可将其从 0 修改至 1，则消耗金币远程兑换允许发弹量。此后哨兵可将其从 1 修改至 2，以此类推。
        uint8_t remote_exchange_blood_count : 4;                ///< 哨兵远程兑换血量的请求次数，开局为 0，修改此值即可请求远程兑换血量
                                                                ///< 此值的变化需要单调递增且每次仅能增加 1，否则视为不合法。
                                                                ///< 示例：此值开局仅能为 0，此后哨兵可将其从 0 修改至 1，则消耗金币远程兑换血量。此后哨兵可将其从 1 修改至 2，以此类推。

        SentryPosture current_posture : 2;  ///< 哨兵修改当前姿态指令，1 为进攻姿态，2 为防御姿态，3 为移动姿态，默认为 3；修改此值即可改变哨兵姿态。
        bool activating_energy_machine : 1; ///< 哨兵机器人是否确认使能量机关进入正在激活状态，true 为确认。默认为 false。

        uint8_t : 8; ///< 保留位
    };

    /**
     * @brief 0x0301 | 0x0121
     * @note 仅开局和每次对方破解成功使得加密等级（己方干扰波难度）提高时可以修改密钥，其余时间修改无效。
     * @note 当 password_cmd 值为 2 时，每次更新验证密钥后的 10 秒内，再次更新无效。
     */
    struct RaderCmd final
    {
        uint8_t radar_cmd;    ///< 开局为 0，修改此值即可请求触发双倍易伤，若此时雷达拥有触发双倍易伤的机会，则可触发。
                              ///< 此值的变化需要单调递增且每次仅能增加 1，否则视为不合法。
                              ///< 示例：此值开局仅能为 0，此后雷达可将其从 0 修改至 1，若雷达拥有触发双倍易伤的机会，则触发双倍易伤。此后雷达可将其从 1 修改至 2，以此类推。
                              ///< 若雷达请求双倍易伤时，双倍易伤正在生效，则第二次双倍易伤将在第一次双倍易伤结束后生效。
        uint8_t password_cmd; ///< 指令类型
                              ///< 当 password_cmd 值为 1 时，修改此值即可更新己方加密密钥；当 password_cmd 值为 2 时，修改此值即可将雷达破解的对方密钥传输给服务器以验证是否正确破解。
        uint8_t password[6];  ///< 每个字节均为 ASCII 码编码的字母或数字。开局为随机值。
    };

    /**
     * @brief 0x0301 | 0x0F01 设置图传出图信道
     */

    /**
     * @brief 0x0301 | 0x0F02 设置图传出图信道
     */

    /* -------------------- 小地图交互数据 -------------------- */

    /* -------------------- 图传链路 -------------------- */

    /**
     * @brief 0x0302 操作手可使用自定义控制器通过图传链路向对应的机器人发送数据。
     */
    template <std::size_t N>
        requires(N <= 30)
    struct CustomRobotData final
    {
        uint8_t data[N]; ///< 自定义数据
    };

    /**
     * @brief 0x0309 机器人可通过图传链路向对应的操作手选手端连接的自定义控制器发送数据（RMUL 暂不适用）。
     */
    template <std::size_t N>
        requires(N <= 30)
    struct CustomRobotData2 final
    {
        uint8_t data[N]; ///< 自定义数据
    };

    /**
     * @brief 0x0310
     */
    template <std::size_t N>
        requires(N <= 150)
    struct CustomRobotData3 final
    {
        uint8_t data[N]; ///< 自定义数据
    };

    /**
     * @brief 0x0304
     */
    struct RemoteControl final
    {
        int16_t mouse_x;          ///< 鼠标 x 轴移动速度，负值标识向左移动
        int16_t mouse_y;          ///< 鼠标 y 轴移动速度，负值标识向下移动
        int16_t mouse_z;          ///< 鼠标滚轮移动速度，负值标识向后滚动
        int8_t left_button_down;  ///< 鼠标左键是否按下：0 为未按下；1 为按下
        int8_t right_button_down; ///< 鼠标右键是否按下：0 为未按下，1 为按下
        uint16_t keyboard_value;  ///< 键盘按键信息，每个 bit 对应一个按键，0 为未按下，1 为按下：
                                  ///< W S A D Shift Ctrl Q E R F G Z X C V B
        uint16_t : 16;            ///< 保留位
    };

    /* -------------------- 非链路数据 -------------------- */
#pragma pack(pop)

    namespace vision
    {
#pragma pack(push, 1)

        /**
         * @brief 图传自带协议结构体
         */
        struct VisionRemoteControl final
        {
            static constexpr uint16_t FRAME_SOF_1 = 0xA9;
            static constexpr uint16_t FRAME_SOF_2 = 0x53;

            static constexpr uint16_t CHANNEL_MIDPOINT = 1024; ///< 通道中点值
            static constexpr uint16_t CHANNEL_MAX = 1684;      ///< 通道最大值
            static constexpr uint16_t CHANNEL_MIN = 364;       ///< 通道最小值

            uint16_t sof_1; ///< 帧头1，固定为 0xA9
            uint16_t sof_2; ///< 帧头2，固定为 0x53

            uint16_t channel_0 : 11; ///< 通道 0，取值范围 364~1684
            uint16_t channel_1 : 11; ///< 通道 1，取值范围 364~1684
            uint16_t channel_2 : 11; ///< 通道 2，取值范围 364~1684
            uint16_t channel_3 : 11; ///< 通道 3，取值范围 364~1684

            uint16_t switch_mode : 2; ///< 挡位切换开关 C：0 | N：1 | S：2
            bool switch_pause : 1;    ///< 暂停开关：按下 1 | 未按下 0
            bool switch_fn_l : 1;     ///< 左功能开关：按下 1 | 未按下 0
            bool switch_fn_r : 1;     ///< 右功能开关：按下 1 | 未按下 0
            uint16_t dial : 11;       ///< 拨轮，取值范围 364~1684
            bool trigger : 1;         ///< 扳机开关：按下 1 | 未按下 0

            int16_t mouse_x; ///< 鼠标 x 轴移动速度，负值标识向左移动
            int16_t mouse_y; ///< 鼠标 y 轴移动速度，负值标识向下移动
            int16_t mouse_z; ///< 鼠标滚轮移动速度，负值标识向后滚动

            uint8_t mouse_l : 2; ///< 鼠标左键状态：0 为未按下；1 为按下
            uint8_t mouse_r : 2; ///< 鼠标右键状态：0 为未按下；1 为按下
            uint8_t mouse_m : 2; ///< 鼠标中键状态：0 为未按下；1 为按下

            struct
            {
                bool w : 1;     ///< W键
                bool s : 1;     ///< S键
                bool d : 1;     ///< D键
                bool a : 1;     ///< A键
                bool shift : 1; ///< Shift键
                bool ctrl : 1;  ///< Ctrl键
                bool q : 1;     ///< Q键
                bool e : 1;     ///< E键
                bool r : 1;     ///< R键
                bool f : 1;     ///< F键
                bool g : 1;     ///< G键
                bool z : 1;     ///< Z键
                bool x : 1;     ///< X键
                bool c : 1;     ///< C键
                bool v : 1;     ///< V键
                bool b : 1;     ///< B键
            } key;

            uint16_t crc16; ///< CRC16 校验码
        };
#pragma pack(pop)
    } // namespace vision
} // namespace referee
