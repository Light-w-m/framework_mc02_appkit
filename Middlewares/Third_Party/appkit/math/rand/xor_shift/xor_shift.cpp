#include <xor_shift.hpp>

namespace appkit::math
{
    XorShift::XorShift(const uint32_t seed) : seed_(seed)
    {
    }

    uint32_t XorShift::Get()
    {
        seed_ ^= (seed_ << 13) & UINT32_MAX;
        seed_ ^= (seed_ >> 17);
        seed_ ^= (seed_ << 5) & UINT32_MAX;

        return seed_;
    }
} // namespace appkit::math
