#include <mit.hpp>

namespace algorithm
{
    float MIT::Update(float pos_target, float spd_target, float pos_measure, float spd_measure, float torque_ff) noexcept
    {
        // 计算位置误差和速度误差
        float pos_error = pos_target - pos_measure;
        float spd_error = spd_target - spd_measure;

        // 比例项
        pOut_ = param_.kp * pos_error;

        // 微分项
        dOut_ = param_.kd * spd_error;

        // 计算总输出
        float output = pOut_ + dOut_ + torque_ff;

        // 最大输出限幅
        f_MaxOutput(output);

        return output;
    }

    void MIT::Reset() noexcept
    {
        pOut_ = 0.0f;
        dOut_ = 0.0f;
    }

    void MIT::f_MaxOutput(float &output) const noexcept
    {
        const float &max_output = param_.max_output;

        if (max_output > 0.0f)
        {
            output = std::clamp(output, -max_output, max_output);
        }
    }
} // namespace algorithm
