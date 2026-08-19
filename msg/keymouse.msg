% namespace control
% pack 4

% alias KeymouseCtrlData
% comment "@brief 键鼠数据结构体"

# 鼠标数据
float32 mouse_speed_x   # 鼠标X轴速度 映射到[-1, +1]
float32 mouse_speed_y   # 鼠标Y轴速度 映射到[-1, +1]
float32 mouse_speed_z   # 鼠标Z轴速度 映射到[-1, +1]

bool press_l    # 鼠标左键是否按下
bool press_r    # 鼠标右键是否按下
bool press_m    # 鼠标中键是否按下

# 键盘数据
bool w      # 键盘W键是否按下
bool s      # 键盘S键是否按下
bool d      # 键盘D键是否按下
bool a      # 键盘A键是否按下
bool shift  # 键盘Shift键是否按下
bool ctrl   # 键盘Ctrl键是否按下
bool q      # 键盘Q键是否按下
bool e      # 键盘E键是否按下
bool r      # 键盘R键是否按下
bool f      # 键盘F键是否按下
bool g      # 键盘G键是否按下
bool z      # 键盘Z键是否按下
bool x      # 键盘X键是否按下
bool c      # 键盘C键是否按下
bool v      # 键盘V键是否按下
bool b      # 键盘B键是否按下