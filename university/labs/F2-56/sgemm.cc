// sgemm.cc - C = A * B in single precision, step by step, with a speed table.
// Row-major: A is M x K, B is K x N, C is M x N. Every step is checked for correctness.
#include "bench.hpp"
#include <immintrin.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <random>
#include <thread>
#include <vector>

using Mat = std::vector<float>;
using Kernel = void (*)(Mat const&, Mat const&, Mat&, std::size_t, std::size_t, std::size_t);

// Step 1: the textbook triple loop. The inner loop walks down a column of B.
void naiveIJK(Mat const& A, Mat const& B, Mat& C, std::size_t M, std::size_t N, std::size_t K)
{
    for (std::size_t i = 0; i < M; ++i) {
        for (std::size_t j = 0; j < N; ++j) {
            float s = 0.0f;
            for (std::size_t k = 0; k < K; ++k) {
                s += A[i * K + k] * B[k * N + j];
            }
            C[i * N + j] = s;
        }
    }
}

// Step 2: swap the loops. The inner loop now walks along rows of B and C (unit stride),
// which the cache likes and the compiler can vectorise.
void loopIKJ(Mat const& A, Mat const& B, Mat& C, std::size_t M, std::size_t N, std::size_t K)
{
    std::fill(C.begin(), C.end(), 0.0f);
    for (std::size_t i = 0; i < M; ++i) {
        for (std::size_t k = 0; k < K; ++k) {
            float const a = A[i * K + k];
            for (std::size_t j = 0; j < N; ++j) {
                C[i * N + j] += a * B[k * N + j];
            }
        }
    }
}

// Step 3: cache blocking. Work on a KC x NC panel of B at a time so it stays in cache
// while every row of A uses it.
constexpr std::size_t KC = 256;
constexpr std::size_t NC = 512;

void blocked(Mat const& A, Mat const& B, Mat& C, std::size_t M, std::size_t N, std::size_t K)
{
    std::fill(C.begin(), C.end(), 0.0f);
    for (std::size_t j0 = 0; j0 < N; j0 += NC) {
        std::size_t const j1 = std::min(j0 + NC, N);
        for (std::size_t k0 = 0; k0 < K; k0 += KC) {
            std::size_t const k1 = std::min(k0 + KC, K);
            for (std::size_t i = 0; i < M; ++i) {
                for (std::size_t k = k0; k < k1; ++k) {
                    float const a = A[i * K + k];
                    for (std::size_t j = j0; j < j1; ++j) {
                        C[i * N + j] += a * B[k * N + j];
                    }
                }
            }
        }
    }
}

// Step 4: register tiling. A 6 x 16 block of C lives in 12 AVX2 registers for a whole
// k-panel: each step loads 16 floats of B once and reuses them for 6 rows of A.
constexpr std::size_t MR = 6;
constexpr std::size_t NR = 16;

[[gnu::target("avx2,fma")]] void microKernel(float const* A, float const* B, float* C,
                                             std::size_t N, std::size_t K, std::size_t kLen)
{
    __m256 c[MR][2];
    for (std::size_t r = 0; r < MR; ++r) {
        c[r][0] = _mm256_loadu_ps(C + r * N);
        c[r][1] = _mm256_loadu_ps(C + r * N + 8);
    }
    for (std::size_t k = 0; k < kLen; ++k) {
        __m256 const b0 = _mm256_loadu_ps(B + k * N);
        __m256 const b1 = _mm256_loadu_ps(B + k * N + 8);
#pragma GCC unroll 6  // keep all 12 accumulators in registers
        for (std::size_t r = 0; r < MR; ++r) {
            __m256 const a = _mm256_broadcast_ss(A + r * K + k);
            c[r][0] = _mm256_fmadd_ps(a, b0, c[r][0]);
            c[r][1] = _mm256_fmadd_ps(a, b1, c[r][1]);
        }
    }
    for (std::size_t r = 0; r < MR; ++r) {
        _mm256_storeu_ps(C + r * N, c[r][0]);
        _mm256_storeu_ps(C + r * N + 8, c[r][1]);
    }
}

// Rows [i0, i1) of C; edges that do not fill a 6 x 16 tile use the simple loop.
void registerTiledRows(Mat const& A, Mat const& B, Mat& C, std::size_t N, std::size_t K,
                       std::size_t i0, std::size_t i1)
{
    std::fill(C.begin() + static_cast<std::ptrdiff_t>(i0 * N),
              C.begin() + static_cast<std::ptrdiff_t>(i1 * N), 0.0f);
    std::size_t const iFull = i0 + (i1 - i0) / MR * MR;
    std::size_t const jFull = N / NR * NR;
    for (std::size_t k0 = 0; k0 < K; k0 += KC) {
        std::size_t const kLen = std::min(KC, K - k0);
        for (std::size_t i = i0; i < iFull; i += MR) {
            for (std::size_t j = 0; j < jFull; j += NR) {
                microKernel(&A[i * K + k0], &B[k0 * N + j], &C[i * N + j], N, K, kLen);
            }
        }
        for (std::size_t i = i0; i < i1; ++i) {  // edge columns (all rows) and edge rows
            std::size_t const jStart = (i < iFull) ? jFull : 0;
            for (std::size_t k = k0; k < k0 + kLen; ++k) {
                float const a = A[i * K + k];
                for (std::size_t j = jStart; j < N; ++j) {
                    C[i * N + j] += a * B[k * N + j];
                }
            }
        }
    }
}

void registerTiled(Mat const& A, Mat const& B, Mat& C, std::size_t M, std::size_t N, std::size_t K)
{
    registerTiledRows(A, B, C, N, K, 0, M);
}

// Step 5: the same kernel on every CPU; each thread owns a band of rows of C.
void threaded(Mat const& A, Mat const& B, Mat& C, std::size_t M, std::size_t N, std::size_t K)
{
    std::size_t const T = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> pool;
    for (std::size_t t = 0; t < T; ++t) {
        std::size_t const i0 = M * t / T / MR * MR;
        std::size_t const i1 = (t + 1 == T) ? M : M * (t + 1) / T / MR * MR;
        pool.emplace_back([&, i0, i1] { registerTiledRows(A, B, C, N, K, i0, i1); });
    }
    for (auto& th : pool) {
        th.join();
    }
}

// Accept if |C - ref| <= 2 * K * u * sum_k |a_ik||b_kj| for every element (u = 2^-24),
// a standard worst-case bound for a float dot product of length K.
bool check(Mat const& A, Mat const& B, Mat const& C, std::size_t M, std::size_t N, std::size_t K,
           double& worst)
{
    worst = 0.0;
    for (std::size_t i = 0; i < M; ++i) {
        for (std::size_t j = 0; j < N; ++j) {
            double ref = 0.0;
            double mag = 0.0;
            for (std::size_t k = 0; k < K; ++k) {
                double const p = double(A[i * K + k]) * double(B[k * N + j]);
                ref += p;
                mag += std::fabs(p);
            }
            double const bound = 2.0 * double(K) * std::ldexp(1.0, -24) * mag;
            double const ratio = std::fabs(double(C[i * N + j]) - ref) / (bound > 0 ? bound : 1.0);
            worst = std::max(worst, ratio);
        }
    }
    return worst <= 1.0;
}

Mat randomMatrix(std::size_t rows, std::size_t cols, unsigned seed)
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> d(-1.0f, 1.0f);
    Mat m(rows * cols);
    for (auto& x : m) {
        x = d(rng);
    }
    return m;
}

struct Step
{
    char const* name;
    Kernel kernel;
};

int main()
{
    Step const steps[] = {{"1 naive ijk", naiveIJK}, {"2 loop order ikj", loopIKJ},
                          {"3 cache blocked", blocked}, {"4 register tiled", registerTiled},
                          {"5 threads", threaded}};
    // Correctness on awkward shapes first (curriculum E6 acceptance test, CPU version).
    std::size_t const shapes[][3] = {{1, 1, 1}, {7, 5, 3}, {100, 37, 61}, {513, 17, 300},
                                     {17, 600, 513}, {128, 128, 128}, {300, 301, 302}};
    std::printf("correctness (worst error / bound; pass if <= 1)\n");
    bool allOk = true;
    for (auto const& s : shapes) {
        Mat const A = randomMatrix(s[0], s[2], 1);
        Mat const B = randomMatrix(s[2], s[1], 2);
        std::printf("  M=%3zu N=%3zu K=%3zu:", s[0], s[1], s[2]);
        for (auto const& st : steps) {
            Mat C(s[0] * s[1], -7.0f);
            st.kernel(A, B, C, s[0], s[1], s[2]);
            double worst = 0.0;
            bool const ok = check(A, B, C, s[0], s[1], s[2], worst);
            allOk = allOk && ok;
            std::printf(" %.3f%s", worst, ok ? "" : "(FAIL)");
        }
        std::printf("\n");
    }
    std::printf("all shapes, all steps: %s\n\n", allOk ? "PASS" : "FAIL");
    // Speed table.
    for (std::size_t n : {std::size_t{512}, std::size_t{1024}}) {
        Mat const A = randomMatrix(n, n, 3);
        Mat const B = randomMatrix(n, n, 4);
        Mat C(n * n);
        double const flops = 2.0 * double(n) * double(n) * double(n);
        double base = 0.0;
        std::printf("M = N = K = %zu, median of 21 runs (3 warm-ups)\n", n);
        std::printf("  %-18s %10s %10s %10s\n", "step", "ms", "GFLOP/s", "speed-up");
        for (auto const& st : steps) {
            if (n > 512 && st.kernel == naiveIJK) {
                std::printf("  %-18s %10s %10s %10s\n", st.name, "skipped", "-", "-");
                continue;
            }
            auto const t = bench::run([&] { st.kernel(A, B, C, n, n, n); bench::keep(C[0]); });
            double const g = flops / t.median / 1e9;
            if (base == 0.0) {
                base = t.median;
            }
            std::printf("  %-18s %10.2f %10.2f %10.1f\n", st.name, t.median * 1e3, g, base / t.median);
            // compulsory traffic: read A and B once, write C once (4 bytes per float)
            double const bytes = 4.0 * (3.0 * double(n) * double(n));
            unsigned const threads = (st.kernel == threaded) ? std::thread::hardware_concurrency() : 1;
            std::printf("KERNEL sgemm%zu_step%c %u %.0f %.0f %.3f\n", n, st.name[0], threads, flops, bytes, g);
        }
        std::printf("\n");
    }
    return allOk ? 0 : 1;
}
