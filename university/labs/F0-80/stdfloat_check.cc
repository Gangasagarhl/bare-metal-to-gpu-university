// F0-80 cross-check: does the model in lowp.hpp round exactly like GCC's std::float16_t and
// std::bfloat16_t on this machine? (A .cc file, so run_lab.sh skips it; run.sh builds it with
// -std=c++23.)
#include <cstdio>
#include <limits>
#include <stdfloat>

#include "lowp.hpp"

template <typename T> void describe(const char* name)
{
    using L = std::numeric_limits<T>;
    std::printf("%-17s digits=%d max=%.9g min=%.9g denorm_min=%.9g epsilon=%.9g\n", name, L::digits,
                static_cast<double>(L::max()), static_cast<double>(L::min()),
                static_cast<double>(L::denorm_min()), static_cast<double>(L::epsilon()));
}

int main()
{
    describe<std::float16_t>("std::float16_t");
    describe<std::bfloat16_t>("std::bfloat16_t");
    std::uint64_t state = 7;
    long tested = 0, diff16 = 0, diffbf = 0;
    for (int i = 0; i < 200000; ++i) {
        // values spread over many binades, both signs, including subnormal and overflow ranges
        const double m = uniformMinus1To1(state);
        const int e = static_cast<int>(splitmix64(state) % 300) - 150;
        const float x =
            static_cast<float>(std::ldexp(m, e)); // the hardware types convert from float here
        const double ref16 = static_cast<double>(static_cast<std::float16_t>(x));
        const double refbf = static_cast<double>(static_cast<std::bfloat16_t>(x));
        diff16 += (roundTo(static_cast<double>(x), kBinary16) != ref16) ? 1 : 0;
        diffbf += (roundTo(static_cast<double>(x), kBfloat16) != refbf) ? 1 : 0;
        ++tested;
    }
    std::printf(
        "values tested: %ld; model differs from std::float16_t: %ld; from std::bfloat16_t: %ld\n",
        tested, diff16, diffbf);
    return (diff16 == 0 && diffbf == 0) ? 0 : 1;
}
