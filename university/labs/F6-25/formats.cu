// F6-25 Listing 1: the reduced-precision formats, measured instead of remembered.
// Part 1 walks every bit pattern of FP16, BF16, FP8 E4M3 and FP8 E5M2 through NVIDIA's
// own conversion functions (cuda_fp16.h, cuda_bf16.h, cuda_fp8.h; host code, no GPU) and
// reports the largest finite value, the smallest positive value and how many are finite.
// Part 2 checks our rounding model (lowp.hpp) against NVIDIA's float -> format conversion
// for a million random values inside each format's range.
#include <cmath>
#include <cstdio>
#include <random>
#include <cuda_bf16.h>
#include <cuda_fp16.h>
#include <cuda_fp8.h>
#include "lowp.hpp"

float fromBits16(unsigned bits, bool bf16)
{
    if (bf16) {
        __nv_bfloat16_raw r;
        r.x = static_cast<unsigned short>(bits);
        return __bfloat162float(__nv_bfloat16(r));
    }
    __half_raw r;
    r.x = static_cast<unsigned short>(bits);
    return __half2float(__half(r));
}

template <typename Fp8>
float fromBits8(unsigned bits)
{
    Fp8 v;
    v.__x = static_cast<__nv_fp8_storage_t>(bits);
    return float(v);
}

template <typename Decode>
void survey(const char* name, unsigned patterns, Decode decode)
{
    float largest = 0.0f, smallest = INFINITY;
    unsigned finite = 0, nans = 0, infs = 0;
    for (unsigned b = 0; b < patterns; ++b) {
        const float v = decode(b);
        if (std::isnan(v)) {
            ++nans;
        } else if (std::isinf(v)) {
            ++infs;
        } else {
            ++finite;
            largest = std::fmax(largest, v);
            if (v > 0.0f) {
                smallest = std::fmin(smallest, v);
            }
        }
    }
    std::printf("%-9s %6u patterns: %6u finite, %3u infinite, %4u NaN; largest finite %-12.7g "
                "smallest positive %.7g\n", name, patterns, finite, infs, nans, largest, smallest);
}

template <typename Convert>
void compare(const Format& f, double lo, double hi, Convert nvidia)
{
    std::mt19937 gen(7u);
    std::uniform_real_distribution<double> mag(std::log2(lo), std::log2(hi));
    std::uniform_int_distribution<int> sign(0, 1);
    int agree = 0;
    const int n = 1000000;
    double worstRel = 0.0;
    for (int i = 0; i < n; ++i) {
        const float x = float((sign(gen) ? -1.0 : 1.0) * std::exp2(mag(gen)));
        const double ours = roundTo(x, f);
        agree += ours == double(nvidia(x)) ? 1 : 0;
        if (std::fabs(x) >= std::ldexp(1.0, f.emin)) {     // relative error: normal range only
            worstRel = std::fmax(worstRel, std::fabs(ours - x) / std::fabs(x));
        }
    }
    std::printf("%-9s model agrees with NVIDIA's conversion on %7d of %d values; "
                "largest relative error %.6g (u = 2^-%d = %.6g)\n",
                f.name, agree, n, worstRel, f.p, unitRoundoff(f));
}

int main()
{
    std::printf("Part 1: every bit pattern decoded by NVIDIA's conversion functions\n");
    survey("FP16", 1u << 16, [](unsigned b) { return fromBits16(b, false); });
    survey("BF16", 1u << 16, [](unsigned b) { return fromBits16(b, true); });
    survey("FP8 E4M3", 1u << 8, fromBits8<__nv_fp8_e4m3>);
    survey("FP8 E5M2", 1u << 8, fromBits8<__nv_fp8_e5m2>);

    std::printf("\nPart 2: our rounding model against NVIDIA's float -> format conversion\n");
    compare(FP16, 1e-7, 6e4, [](float x) { return __half2float(__float2half_rn(x)); });
    compare(BF16, 1e-38, 1e38, [](float x) { return __bfloat162float(__float2bfloat16_rn(x)); });
    compare(E4M3, 1e-3, 440.0, [](float x) { return float(__nv_fp8_e4m3(x)); });
    compare(E5M2, 1e-5, 5e4, [](float x) { return float(__nv_fp8_e5m2(x)); });
    std::printf("TF32      no host conversion in this toolkit's headers was used; the model "
                "(p = 11, FP32 exponent range) is not checked here\n");

    std::printf("\nPart 3: three values through each conversion (FP8 conversions saturate to "
                "the largest finite value)\n");
    for (float x : {3.14159265f, 70000.0f, 1e-8f}) {
        std::printf("x = %-11.8g FP16 %-12.8g BF16 %-12.8g E4M3 %-10.6g E5M2 %-10.6g\n", x,
                    __half2float(__float2half_rn(x)), __bfloat162float(__float2bfloat16_rn(x)),
                    float(__nv_fp8_e4m3(x)), float(__nv_fp8_e5m2(x)));
    }
    return 0;
}
