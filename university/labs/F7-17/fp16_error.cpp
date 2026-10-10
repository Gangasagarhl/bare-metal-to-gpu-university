// F7-17 Listing 2: where the error of a mixed-precision GEMM comes from (E7 tolerance).
// For one dot product of length K it compares, against an FP64 reference of the original values:
//   (1) inputs rounded to FP16, products summed in FP64   -> input rounding only
//   (2) inputs rounded to FP16, products summed in FP32   -> what an FP16/FP32 matrix unit computes
//   (3) inputs rounded to BF16, products summed in FP32
// Errors are printed relative to S = sum |a_k b_k|, the scale every bound uses.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

float toBf16(float x)                          // round to nearest even, keep the top 16 bits
{
    std::uint32_t u;
    std::memcpy(&u, &x, 4);
    u += 0x7FFF + ((u >> 16) & 1);
    u &= 0xFFFF0000u;
    float r;
    std::memcpy(&r, &u, 4);
    return r;
}

int main()
{
    std::mt19937 gen(17);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    const int Ks[] = {64, 512, 4096};
    const int trials = 200;
    const double u16 = std::ldexp(1.0, -11), ub16 = std::ldexp(1.0, -8), u32 = std::ldexp(1.0, -24);
    std::printf("unit roundoff: FP16 %.3g, BF16 %.3g, FP32 %.3g\n", u16, ub16, u32);
    std::printf("%-6s %-14s %-14s %-14s %-12s %-12s\n", "K", "(1) fp16 in", "(2) fp16+fp32",
                "(3) bf16+fp32", "bound fp16", "bound bf16");
    for (int K : Ks) {
        double worst1 = 0, worst2 = 0, worst3 = 0;
        for (int t = 0; t < trials; ++t) {
            std::vector<float> a(K), b(K);
            for (int k = 0; k < K; ++k) {
                a[k] = dist(gen);
                b[k] = dist(gen);
            }
            double ref = 0, S = 0, s1 = 0;
            float s2 = 0.0f, s3 = 0.0f;
            for (int k = 0; k < K; ++k) {
                ref += static_cast<double>(a[k]) * b[k];
                S += std::fabs(static_cast<double>(a[k]) * b[k]);
                const float ha = static_cast<float>(static_cast<_Float16>(a[k]));
                const float hb = static_cast<float>(static_cast<_Float16>(b[k]));
                s1 += static_cast<double>(ha) * hb;
                s2 += ha * hb;                 // FP16 x FP16 is exact in FP32 (11 + 11 <= 24 bits)
                s3 += toBf16(a[k]) * toBf16(b[k]);
            }
            worst1 = std::fmax(worst1, std::fabs(s1 - ref) / S);
            worst2 = std::fmax(worst2, std::fabs(s2 - ref) / S);
            worst3 = std::fmax(worst3, std::fabs(s3 - ref) / S);
        }
        // first-order bounds: each product carries (2u + u^2) from its two rounded inputs,
        // and a K-term FP32 sum adds at most K * u32 (relative to S)
        const double b16 = 2 * u16 + u16 * u16 + K * u32;
        const double bb16 = 2 * ub16 + ub16 * ub16 + K * u32;
        std::printf("%-6d %-14.3e %-14.3e %-14.3e %-12.3e %-12.3e\n", K, worst1, worst2, worst3, b16, bb16);
    }
    return 0;
}
