// simd.cc - saxpy (y = a*x + y) and a float sum, scalar versus SIMD.
// run.sh builds this file three ways (-O2 -fno-tree-vectorize, -O3, -O3 -march=native).
#include "bench.hpp"
#include <immintrin.h>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

// Plain C++: whether this becomes SIMD is the compiler's decision (see the build flags).
[[gnu::noinline]] void saxpyPlain(float* y, float const* x, float a, std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i) {
        y[i] = a * x[i] + y[i];
    }
}

// Explicit AVX2 + FMA intrinsics: 8 floats per instruction, whatever the build flags say.
[[gnu::noinline, gnu::target("avx2,fma")]] void saxpyAvx2(float* y, float const* x, float a,
                                                          std::size_t n)
{
    __m256 const va = _mm256_set1_ps(a);
    std::size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m256 const vx = _mm256_loadu_ps(x + i);
        __m256 const vy = _mm256_loadu_ps(y + i);
        _mm256_storeu_ps(y + i, _mm256_fmadd_ps(va, vx, vy));
    }
    for (; i < n; ++i) {  // the remainder, one element at a time
        y[i] = a * x[i] + y[i];
    }
}

[[gnu::noinline]] float sumPlain(float const* x, std::size_t n)
{
    float s = 0.0f;
    for (std::size_t i = 0; i < n; ++i) {
        s += x[i];
    }
    return s;
}

// Eight partial sums: we choose a different (still deterministic) order of additions,
// which leaves the compiler free to put the eight sums in one SIMD register.
[[gnu::noinline]] float sum8(float const* x, std::size_t n)
{
    float p[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    std::size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        for (std::size_t k = 0; k < 8; ++k) {
            p[k] += x[i + k];
        }
    }
    float s = ((p[0] + p[1]) + (p[2] + p[3])) + ((p[4] + p[5]) + (p[6] + p[7]));
    for (; i < n; ++i) {
        s += x[i];
    }
    return s;
}

template <typename F>
double nsPerElement(F f, std::size_t n, int inner)
{
    auto const s = bench::run([&] {
        for (int r = 0; r < inner; ++r) {
            f();
        }
    });
    return s.median * 1e9 / (static_cast<double>(n) * inner);
}

int main()
{
#if defined(__AVX512F__)
    std::puts("build: __AVX512F__ and __AVX2__ defined (compiler may use 16- to 64-byte vectors)");
#elif defined(__AVX2__)
    std::puts("build: __AVX2__ defined");
#else
    std::puts("build: neither __AVX2__ nor __AVX512F__ defined (baseline x86-64: 16-byte SSE)");
#endif
    bool const haveAvx2 = __builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma");
    std::printf("this CPU reports avx2+fma: %s\n", haveAvx2 ? "yes" : "no");
    for (std::size_t n : {std::size_t{4096}, std::size_t{1} << 24}) {
        int const inner = n < 100000 ? 2000 : 1;
        std::vector<float> x(n), y(n, 1.0f), y2(n, 1.0f);
        for (std::size_t i = 0; i < n; ++i) {
            x[i] = static_cast<float>(i % 100) * 0.01f;
        }
        double const tPlain = nsPerElement([&] { saxpyPlain(y.data(), x.data(), 1e-6f, n); }, n, inner);
        double tAvx = 0.0;
        if (haveAvx2) {
            tAvx = nsPerElement([&] { saxpyAvx2(y2.data(), x.data(), 1e-6f, n); }, n, inner);
        }
        // Both arrays received the same number of updates; FMA rounds once, so allow a tiny gap.
        double maxRel = 0.0;
        for (std::size_t i = 0; haveAvx2 && i < n; ++i) {
            maxRel = std::fmax(maxRel, std::fabs(double(y[i]) - double(y2[i])) / std::fabs(double(y[i])));
        }
        double const tSum = nsPerElement([&] { bench::keep(sumPlain(x.data(), n)); }, n, inner);
        double const tSum8 = nsPerElement([&] { bench::keep(sum8(x.data(), n)); }, n, inner);
        std::printf("n = %9zu (%8.1f KiB per array)\n", n, static_cast<double>(n) * 4 / 1024);
        std::printf("  saxpyPlain %7.3f ns/elem  %6.2f GFLOP/s\n", tPlain, 2.0 / tPlain);
        if (haveAvx2) {
            std::printf("  saxpyAvx2  %7.3f ns/elem  %6.2f GFLOP/s  (max relative difference %.1e)\n",
                        tAvx, 2.0 / tAvx, maxRel);
        }
        std::printf("  sumPlain   %7.3f ns/elem\n", tSum);
        std::printf("  sum8       %7.3f ns/elem\n", tSum8);
    }
    return 0;
}
