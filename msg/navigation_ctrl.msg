% namespace control
% pack 4

% alias NaviCtrlData
% comment "@brief 导航控制数据结构体"

bool is_online  # 在线标志位

float32 vx  # X轴线速度 m/s
float32 vy  # Y轴线速度 m/s
float32 wz  # Z轴角速度 rad/s