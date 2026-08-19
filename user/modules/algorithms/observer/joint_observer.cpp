#include <joint_observer.hpp>

namespace algorithm
{
    JointObserver::JointObserver(const float rls_lambda, const float rls_delta, const float tau_epsilon) noexcept
        : rls_{rls_lambda, rls_delta}, tau_epsilon_(tau_epsilon)
    {
    }

    constexpr const appkit::math::Vector<float, 4> &JointObserver::Update(
        const float angle, const float omega, const float torque, const appkit::Duration dt) noexcept
    {
        // 计算角加速度
        beta_ = (omega - last_omega_) / appkit::DurationCast<appkit::second, float>(dt) * 0.9f + beta_ * 0.1f;
        last_omega_ = omega;

        // 构建输入向量 [beta, omega, SignSat(omega), sin(angle)]
        appkit::math::Vectorf<4> input_vector{
            beta_,          // 角加速度
            omega,          // 角速度
            SignSat(omega), // 角速度的饱和值
            std::sin(angle) // 关节角度的正弦值
        };

        // 使用RLS算法更新权重向量
        return rls_.Update(input_vector, torque);
    }

    constexpr float JointObserver::SignSat(const float value) const noexcept
    {
        if (std::abs(value) > tau_epsilon_)
        {
            return (value > 0.0f) ? 1.0f : -1.0f;
        }
        else
        {
            return value / tau_epsilon_;
        }
    }
} // namespace algorithm
