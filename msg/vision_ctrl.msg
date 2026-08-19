% namespace control
% pack 4

% alias VisionCtrlData
% comment "@brief 自瞄控制数据结构体"

bool is_online      # 在线标志位
bool is_tracking    # 瞄准状态标志位

float32 aim_droll   # 目标Roll角度偏移量 映射到[-Pi, +Pi]
float32 aim_dpitch  # 目标Pitch角度偏移量 映射到[-Pi, +Pi]
float32 aim_dyaw    # 目标Yaw角度偏移量 映射到[-Pi, +Pi]

float32 aim_drift   # 当前瞄准偏移率 映射到[0, +1]