% namespace control
% pack 4

% alias RemoteCtrlData
% comment "@brief 遥控器数据结构体"

# 摇杆值
# 映射到[-1, +1]
float32 rocker_l1   # 左摇杆竖直方向值 映射到[-1, +1]
float32 rocker_l_   # 左摇杆水平方向值 映射到[-1, +1]
float32 rocker_r1   # 右摇杆竖直方向值 映射到[-1, +1]
float32 rocker_r_   # 右摇杆水平方向值 映射到[-1, +1]

# 二挡开关值
# 上: 1 | 下: -1
int8 switch_d1  # 一号二挡开关拨码值 上: 1 | 下: -1
int8 switch_d2  # 二号二挡开关拨码值 上: 1 | 下: -1
int8 switch_d3  # 三号二挡开关拨码值 上: 1 | 下: -1
int8 switch_d4  # 四号二挡开关拨码值 上: 1 | 下: -1

# 三挡开关值
# 上: 1 | 中: 0 | 下: -1
int8 switch_t1  # 一号三挡开关拨码值 上: 1 | 中: 0 | 下: -1
int8 switch_t2  # 二号三挡开关拨码值 上: 1 | 中: 0 | 下: -1
int8 switch_t3  # 三号三挡开关拨码值 上: 1 | 中: 0 | 下: -1
int8 switch_t4  # 四号三挡开关拨码值 上: 1 | 中: 0 | 下: -1

# 附加摇杆值
# 映射到[-1, +1]
float32 rocker_e1   # 一号附加摇杆值 映射到[-1, +1]
float32 rocker_e2   # 二号附加摇杆值 映射到[-1, +1]
