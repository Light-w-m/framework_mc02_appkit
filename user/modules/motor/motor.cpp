#include <motor.hpp>

namespace control::motor
{
    appkit::ErrorCode Motor::TorqueControl(float torque)
    {
        UNUSED(torque);
        return appkit::ErrorCode::NOT_SUPPORTED;
    }
} // namespace control