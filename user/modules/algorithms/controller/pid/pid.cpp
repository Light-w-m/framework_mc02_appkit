#include <pid.hpp>

#include <utility>

namespace algorithm
{
    FORCE_INLINE void PID::f_MaxOutput(float &output) const noexcept
    {
        const float &max_output = param_.max_output;

        if (max_output > 0.0f)
        {
            output = std::clamp(output, -max_output, max_output);
        }
    }

    void PID::f_DeadZone(float &error) const noexcept
    {
        const float &dead_zone = param_.dead_zone;

        if (dead_zone > 0.0f)
        {
            if (error > dead_zone)
                error -= dead_zone;
            else if (error < -dead_zone)
                error += dead_zone;
            else
                error = 0.0f;
        }
    }

    FORCE_INLINE void PID::f_IntegralLimit(float &integral) const noexcept
    {
        const float &integral_limit = param_.integral_limit;

        if (integral_limit > 0.0f)
        {
            integral = std::clamp(integral, -integral_limit, integral_limit);
        }
    }

    void PID::f_IntegralSaturation(float &i_item, float error) const noexcept
    {
        if (param_.use_integral_saturation && param_.max_output > 0.0f)
        {
            // 如果积分项变化量的符号与误差的符号相同，则停止积分
            if ((i_item > 0.0f && error > 0.0f) || (i_item < 0.0f && error < 0.0f))
            {
                i_item = 0.0f;
            }
        }
    }

    float PositionPID::Update(float target, float measure, const appkit::Duration &dt) noexcept
    {
        // 计算误差
        float error = target - measure;
        f_DeadZone(error);

        // 获取时间差，单位秒
        float delta_time = appkit::DurationCast<appkit::second, float>(dt);
        if (delta_time <= 0.0f)
        {
            // 时间差错误，清空积分并返回0
            integral_ = 0.0f;
            return 0.0f;
        }

        // 比例项
        pOut_ = param_.kp * error;

        // 积分项
        if (param_.ki != 0.0f)
        {
            float i_iterm = param_.ki * (error + lastError_) * delta_time / 2.0f;

            // 积分饱和处理
            f_IntegralSaturation(i_iterm, error);

            iOut_ += i_iterm;
        }
        else
        {
            // 积分系数为0，清空积分
            integral_ = 0.0f;
            iOut_ = 0.0f;
        }

        // 积分限幅
        f_IntegralLimit(iOut_);

        // 微分项
        if (param_.use_derivative_measure)
        {
            dOut_ = -param_.kd * (measure - lastMeasure_) / delta_time; // 测量值的微分
        }
        else
        {
            dOut_ = param_.kd * (error - lastError_) / delta_time; // 误差的微分
        }

        // 输出
        float output = pOut_ + iOut_ + dOut_;
        f_MaxOutput(output);

        // 保存状态
        lastError_ = error;
        lastMeasure_ = measure;

        return output;
    }

    void PositionPID::Reset() noexcept
    {
        lastError_ = 0.0f;
        lastMeasure_ = 0.0f;
        integral_ = 0.0f;
        pOut_ = 0.0f;
        iOut_ = 0.0f;
        dOut_ = 0.0f;
    }

    float IncrementalPID::Update(float target, float measure, const appkit::Duration &dt) noexcept
    {
        // 计算误差
        float error = target - measure;
        f_DeadZone(error);

        // 获取时间差，单位秒
        float delta_time = appkit::DurationCast<appkit::second, float>(dt);
        if (delta_time <= 0.0f)
        {
            // 时间差错误，清空输出
            output_ = 0.0f;
            return output_;
        }

        // 比例项
        pOut_ = param_.kp * (error - lastError_);

        // 积分项
        iOut_ = param_.ki * error * delta_time;

        // 积分饱和处理
        f_IntegralSaturation(iOut_, error);

        // 积分限幅
        f_IntegralLimit(iOut_);

        // 微分项
        if (param_.use_derivative_measure)
        {
            dOut_ = -param_.kd * (measure - 2 * lastMeasure_ + lastMeasure2_) / delta_time; // 测量值的二阶微分
        }
        else
        {
            dOut_ = param_.kd * (error - 2 * lastError_ + lastError2_) / delta_time; // 误差的二阶微分
        }

        // 输出增量
        float output_delta = pOut_ + iOut_ + dOut_;

        // 输出
        output_ += output_delta;
        f_MaxOutput(output_);

        // 保存状态
        lastError2_ = lastError_;
        lastError_ = error;
        lastMeasure2_ = lastMeasure_;
        lastMeasure_ = measure;

        return output_;
    }

    void IncrementalPID::Reset() noexcept
    {
        lastError_ = 0.0f;
        lastError2_ = 0.0f;
        lastMeasure_ = 0.0f;
        lastMeasure2_ = 0.0f;
        integral_ = 0.0f;
        pOut_ = 0.0f;
        iOut_ = 0.0f;
        dOut_ = 0.0f;
        output_ = 0.0f;
    }

    float FuzzyPID::Update(float target, float measure, const appkit::Duration &dt) noexcept
    {
        // 计算误差和误差变化率
        float error = target - measure;
        f_DeadZone(error);
        float delta_error = (error - lastError_) / appkit::DurationCast<appkit::second, float>(dt);
        lastError_ = error;

        // 模糊化误差和误差变化率
        size_t e_set = Fuzzify(error, fuzzyParam_.ke);         // 将误差模糊化到[-3, 3]范围
        size_t ec_set = Fuzzify(delta_error, fuzzyParam_.kec); // 将误差变化率模糊化到[-3, 3]范围

        // 模糊规则推理,解模糊化为PID参数调整量
        float kp_adjust = std::to_underlying(kpFuzzySet[ec_set][e_set]) * fuzzyParam_.kp_scale; // 调整比例系数
        float ki_adjust = std::to_underlying(kiFuzzySet[ec_set][e_set]) * fuzzyParam_.ki_scale; // 调整积分系数
        float kd_adjust = std::to_underlying(kdFuzzySet[ec_set][e_set]) * fuzzyParam_.kd_scale; // 调整微分系数

        // 更新内部位置式PID控制器的参数
        PID::Param &pid_param = fuzzyParam_.baseParam;

        if (fuzzyParam_.max_kp > 0)
            pid_param.kp = std::clamp(pid_param.kp + kp_adjust, 0.0f, fuzzyParam_.max_kp); // 限制比例系数在合理范围
        if (fuzzyParam_.max_ki > 0)
            pid_param.ki = std::clamp(pid_param.ki + ki_adjust, 0.0f, fuzzyParam_.max_ki); // 限制积分系数在合理范围
        if (fuzzyParam_.max_kd > 0)
            pid_param.kd = std::clamp(pid_param.kd + kd_adjust, 0.0f, fuzzyParam_.max_kd); // 限制微分系数在合理范围

        pid_.SetParam(pid_param);

        // 使用更新后的参数计算PID输出
        return pid_.Update(target, measure, dt);
    }

    void FuzzyPID::Reset() noexcept
    {
        fuzzyParam_.baseParam = PID::param_; // 重置为基础PID参数
        pid_.Reset();
        pid_.SetParam(fuzzyParam_.baseParam);
        lastError_ = 0.0f;
    }

    void FuzzyPID::SetFuzzyParam(const FuzzyParam &param) noexcept
    {
        fuzzyParam_ = param;
        pid_.SetParam(fuzzyParam_.baseParam);
    }

    size_t FuzzyPID::Fuzzify(float value, float scale) const noexcept
    {
        ssize_t index = static_cast<ssize_t>(std::round(value * scale));
        return std::clamp(index + 3, 0, 6); // 将[-3, 3]映射到[0, 6]范围
    }
} // namespace algorithm