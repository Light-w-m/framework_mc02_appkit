% namespace sensor_data
% pack 4
% alias AttitudeAngle

float32 timepoint	# 时间戳 ms

float32 roll	    # 欧拉角 Roll rad
float32 pitch	    # 欧拉角 Pitch rad
float32 yaw         # 欧拉角 Yaw rad
float32 yaw_total   # 欧拉角 Yaw 累计值 rad

float32 w	# 四元数 W
float32 x	# 四元数 X
float32 y	# 四元数 Y
float32 z	# 四元数 Z
