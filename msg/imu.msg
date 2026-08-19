% namespace sensor_data
% pack 4

% alias IMURawData
% comment "@brief 六轴IMU原始数据结构体"

float32 timepoint	# 时间戳 ms

float32[3] accel_data	# [x, y, z] 三轴加速度计数据 m/s
float32[3] gyro_data	# [x, y, z] 三轴角速度计数据 rad/s
