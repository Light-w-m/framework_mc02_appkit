#include <lcg.hpp>

namespace appkit::math
{
    LCG::LCG(const uint64_t seed, const uint64_t a, const uint64_t b, const uint64_t m)
        : a_(a), b_(b), m_(m), seed_(seed)
    {
    }

    uint64_t LCG::Get()
    {
        seed_ = (a_ * seed_ + b_) % m_;
        return seed_;
    }
} // namespace appkit::math
