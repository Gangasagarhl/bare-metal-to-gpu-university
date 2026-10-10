// stream.cc - how many bytes per second can one core / all cores move? (the memory roof)
// Triad: a[i] = b[i] + s * c[i]  (2 flops, 12 bytes of loads and stores per element).
#include "bench.hpp"
#include <immintrin.h>
#include <cstddef>
#include <cstdio>
#include <thread>
#include <vector>

[[gnu::noinline, gnu::optimize("no-tree-vectorize")]] void triadScalar(float* a, float const* b,
                                                                        float const* c, float s,
                                                                        std::size_t lo, std::size_t hi)
{
    for (std::size_t i = lo; i < hi; ++i) {
        a[i] = b[i] + s * c[i];
    }
}

[[gnu::noinline, gnu::target("avx2,fma")]] void triadAvx2(float* a, float const* b, float const* c,
                                                          float s, std::size_t lo, std::size_t hi)
{
    __m256 const vs = _mm256_set1_ps(s);
    std::size_t i = lo;
    for (; i + 8 <= hi; i += 8) {
        _mm256_storeu_ps(a + i, _mm256_fmadd_ps(vs, _mm256_loadu_ps(c + i), _mm256_loadu_ps(b + i)));
    }
    for (; i < hi; ++i) {
        a[i] = b[i] + s * c[i];
    }
}

template <typename K>
double gbPerSecond(K kernel, std::vector<float>& a, std::vector<float> const& b,
                   std::vector<float> const& c, int threads, int inner)
{
    std::size_t const n = a.size();
    auto const s = bench::run([&] {
        std::vector<std::thread> pool;
        for (int t = 0; t < threads; ++t) {
            std::size_t const lo = n * t / threads;
            std::size_t const hi = n * (t + 1) / threads;
            pool.emplace_back([&, lo, hi] {
                for (int r = 0; r < inner; ++r) {  // each thread repeats its own slice
                    kernel(a.data(), b.data(), c.data(), 0.5f, lo, hi);
                }
            });
        }
        for (auto& th : pool) {
            th.join();
        }
        bench::keep(a[n / 2]);
    });
    return 12.0 * static_cast<double>(n) * inner / s.median / 1e9;
}

int main()
{
    int const cpus = static_cast<int>(std::thread::hardware_concurrency());
    std::printf("triad a = b + s*c, bytes counted: 12 per element (no write-allocate), median of 21\n");
    std::printf("%-12s %12s %8s %10s\n", "kernel", "array size", "threads", "GB/s");
    double bw1 = 0.0;
    double bwN = 0.0;
    for (std::size_t n : {std::size_t{4096}, std::size_t{1} << 25}) {
        std::vector<float> a(n, 0.0f), b(n, 1.0f), c(n, 2.0f);
        int const inner = n < 100000 ? 20000 : 1;
        int const threads = 1;
        double const gs = gbPerSecond(triadScalar, a, b, c, threads, inner);
        double const gv = gbPerSecond(triadAvx2, a, b, c, threads, inner);
        char size[32];
        std::snprintf(size, sizeof size, "%zu KiB", n * 4 / 1024);
        std::printf("%-12s %12s %8d %10.1f\n", "scalar", size, threads, gs);
        std::printf("%-12s %12s %8d %10.1f\n", "AVX2", size, threads, gv);
        if (n > 100000) {
            bw1 = gv;
            bwN = gbPerSecond(triadAvx2, a, b, c, cpus, 1);
            std::printf("%-12s %12s %8d %10.1f\n", "AVX2", size, cpus, bwN);
            double const flops = 2.0 * static_cast<double>(n);
            double const bytes = 12.0 * static_cast<double>(n);
            std::printf("KERNEL triad_scalar 1 %.0f %.0f %.3f\n", flops, bytes, gs * flops / bytes);
            std::printf("KERNEL triad_avx2 1 %.0f %.0f %.3f\n", flops, bytes, gv * flops / bytes);
        }
    }
    std::printf("ROOF bw1 %.2f\nROOF bwN %.2f\n", bw1, bwN);
    return 0;
}
