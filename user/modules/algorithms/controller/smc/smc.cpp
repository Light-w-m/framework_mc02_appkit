#include <smc.hpp>

#include <math.h>
#include <algorithm>

namespace algorithm
{
    float SMC::Update(float x, float dx, float target, const appkit::Duration &dt) noexcept
    {
        // 获取时间差，单位秒
        float delta_time = appkit::DurationCast<appkit::second, float>(dt);
        if (delta_time <= 0.0f)
        {
            // 时间差错误，清空输出
            u_ = 0.0f;
            return u_;
        }

        x_ = x;
        dx_ = dx;
        // 计算误差
        error_ = x_ - target;
        // 计算滑膜面
        float dref_new = (target - ref_last_) / delta_time;
        float ddref_new = (dref_new - dref_) / delta_time;

        dref_ = dref_new;
        ddref_ = ddref_new;
        ref_last_ = target;

        if (fabs(error_) < param_.error_eps)
        {
            u_ = 0;
            return u_;
        }

        s_ = param_.C * error_ + dx_ - dref_;
        /**
         * @brief 趋近率
         * @note 这里使用指数趋近率，若需要更快的收敛速度，可以使用幂次趋近率
         * @note  k * sgn(s) * |s|^alpha,0<alpha<1
         * @note 这里使用饱和函数是因为考虑为连续系统，若使用符号函数会导致抖动严重
         */
        float u_sw = param_.K * Sat(s_) + param_.epsilon * s_; 

        if (fabs(param_.gx) < 1e-6)
            u_ = 0;
        else        
            u_ = -1 / param_.gx * (u_sw + param_.C * (dx_ - dref_) - ddref_);

        u_ = std::clamp(u_, -param_.u_max, param_.u_max);

        return u_;
    }

    float SMC::Sat(float x) const noexcept
    {
        if (fabs(x) <= param_.phi)
            // return x;
            // 若抖动严重，可以使用陡峭的饱和函数
            return x / param_.phi;
        else 
            return Sign(x);
    }

    float SMC::Sign(float x) const noexcept
    {
        if(x > 0)
            return 1;
        else if(x < 0)
            return -1;
        else
            return 0;
    }

    void SMC::Reset() noexcept
    {
        dref_ = 0;
        ddref_ = 0;
        ref_last_ = 0;
    }
}