// peak.cc - how many single-precision FLOP/s can this CPU really do? (the compute roof)
// Many independent FMA chains keep every FMA unit busy; nothing touches memory.
#include "bench.hpp"
#include <immintrin.h>
#include <cstdio>
#include <thread>
#include <vector>

constexpr int kChains = 12;       // independent accumulators (Little's law: latency x rate)
constexpr long kSteps = 20'000'000;

[[gnu::noinline]] float scalarFma(float a, float b)
{
    float acc[kChains];
    for (int c = 0; c < kChains; ++c) {
        acc[c] = static_cast<float>(c);
    }
    for (long s = 0; s < kSteps / 8; ++s) {
#pragma GCC unroll 12
        for (int c = 0; c < kChains; ++c) {
            acc[c] = acc[c] * a + b;
        }
    }
    float sum = 0.0f;
    for (float v : acc) {
        sum += v;
    }
    return sum;
}

[[gnu::noinline, gnu::target("avx2,fma")]] float avx2Fma(float a, float b)
{
    __m256 acc[kChains];
    for (int c = 0; c < kChains; ++c) {
        acc[c] = _mm256_set1_ps(static_cast<float>(c));
    }
    __m256 const va = _mm256_set1_ps(a);
    __m256 const vb = _mm256_set1_ps(b);
    for (long s = 0; s < kSteps; ++s) {
#pragma GCC unroll 12  // fully unrolled, so the 12 accumulators can live in registers
        for (int c = 0; c < kChains; ++c) {
            acc[c] = _mm256_fmadd_ps(acc[c], va, vb);  // 8 lanes x (multiply + add)
        }
    }
    __m256 t = acc[0];
    for (int c = 1; c < kChains; ++c) {
        t = _mm256_add_ps(t, acc[c]);
    }
    float out[8];
    _mm256_storeu_ps(out, t);
    return out[0] + out[7];
}

[[gnu::noinline, gnu::target("avx512f")]] float avx512Fma(float a, float b)
{
    __m512 acc[kChains];
    for (int c = 0; c < kChains; ++c) {
        acc[c] = _mm512_set1_ps(static_cast<float>(c));
    }
    __m512 const va = _mm512_set1_ps(a);
    __m512 const vb = _mm512_set1_ps(b);
    for (long s = 0; s < kSteps; ++s) {
#pragma GCC unroll 12  // fully unrolled, so the 12 accumulators can live in registers
        for (int c = 0; c < kChains; ++c) {
            acc[c] = _mm512_fmadd_ps(acc[c], va, vb);  // 16 lanes x (multiply + add)
        }
    }
    __m512 t = acc[0];
    for (int c = 1; c < kChains; ++c) {
        t = _mm512_add_ps(t, acc[c]);
    }
    float out[16];
    _mm512_storeu_ps(out, t);
    return out[0] + out[15];
}

template <typename F>
double gflops(F f, double flopsPerCall, int threads)
{
    auto const s = bench::run([&] {
        std::vector<std::thread> pool;
        for (int t = 0; t < threads; ++t) {
            pool.emplace_back([&f] { bench::keep(f(0.999999f, 1e-7f)); });
        }
        for (auto& th : pool) {
            th.join();
        }
    }, 1, 11);
    return flopsPerCall * threads / s.median / 1e9;
}

int main()
{
    int const cpus = static_cast<int>(std::thread::hardware_concurrency());
    double const scalarFlops = 2.0 * kChains * (kSteps / 8);
    double const avx2Flops = 2.0 * 8 * kChains * kSteps;
    double const avx512Flops = 2.0 * 16 * kChains * kSteps;
    std::printf("FP32 FMA throughput, %d independent chains, median of 11 runs\n", kChains);
    std::printf("%-22s %8s %12s\n", "variant", "threads", "GFLOP/s");
    std::printf("%-22s %8d %12.1f\n", "scalar (compiler)", 1, gflops(scalarFma, scalarFlops, 1));
    bool const avx2 = __builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma");
    bool const avx512 = __builtin_cpu_supports("avx512f");
    double p1 = 0.0;
    double pN = 0.0;
    if (avx2) {
        p1 = gflops(avx2Fma, avx2Flops, 1);
        pN = gflops(avx2Fma, avx2Flops, cpus);
        std::printf("%-22s %8d %12.1f\n", "AVX2 + FMA (8 lanes)", 1, p1);
        std::printf("%-22s %8d %12.1f\n", "AVX2 + FMA (8 lanes)", cpus, pN);
    }
    if (avx512) {
        std::printf("%-22s %8d %12.1f\n", "AVX-512 (16 lanes)", 1, gflops(avx512Fma, avx512Flops, 1));
        std::printf("%-22s %8d %12.1f\n", "AVX-512 (16 lanes)", cpus, gflops(avx512Fma, avx512Flops, cpus));
    }
    // machine-readable lines for roofline.cc (AVX2 roofs: the SGEMM kernel uses AVX2)
    std::printf("ROOF peak1 %.2f\nROOF peakN %.2f\nROOF threads %d\n", p1, pN, cpus);
    return 0;
}
