% namespace sensor_data
% pack 4

% alias SuperCapData
% comment "@brief 超级电容数据结构体"

bool is_online      # 在线标志位
uint8 error_code    # 错误枚举值

float32 current_power       # 当前功率 W
float32 available_energy    # 剩余容量 映射到[0, +1]