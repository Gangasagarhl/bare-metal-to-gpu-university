// emu_test.hpp: correctness test of a GEMM kernel run by cuda_shim.hpp on the CPU.
// Same shapes, data and error bound as gemm_harness.cuh, so a kernel that passes here
// has correct indexing, edge handling and barriers; it says nothing about speed.
#pragma once
#include <cmath>
#include <cstdio>
#include <functional>
#include <random>
#include <vector>

using GemmLaunch = std::function<void(const float*, const float*, float*, int, int, int)>;

inline bool emuTestShape(const char* name, const GemmLaunch& launchIt, int M, int N, int K)
{
    std::vector<float> A(size_t(M) * K), B(size_t(K) * N), C(size_t(M) * N, std::nanf(""));
    std::mt19937 gen(1u);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (float& x : A) {
        x = dist(gen);
    }
    for (float& x : B) {
        x = dist(gen);
    }
    launchIt(A.data(), B.data(), C.data(), M, N, K);
    const double u = std::ldexp(1.0, -24);
    const double gamma = K * u / (1.0 - K * u);
    int bad = 0;
    double worst = 0.0;                                 // largest error / bound seen
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            double ref = 0.0, sumAbs = 0.0;
            for (int k = 0; k < K; ++k) {
                const double p = double(A[size_t(i) * K + k]) * B[size_t(k) * N + j];
                ref += p;
                sumAbs += std::fabs(p);
            }
            const double err = std::fabs(C[size_t(i) * N + j] - ref);
            const double bound = gamma * sumAbs + 1e-30;
            if (!(err <= bound)) {                       // also catches NaN (never written)
                ++bad;
            } else if (err / bound > worst) {
                worst = err / bound;
            }
        }
    }
    std::printf("  %-16s M=%4d N=%4d K=%4d : %s (%d elements outside the bound; "
                "largest error %.3f of the bound)\n",
                name, M, N, K, bad ? "FAIL" : "ok", bad, worst);
    return bad == 0;
}

inline int emuTestAll(const char* name, const GemmLaunch& launchIt)
{
    const int shapes[][3] = {{1, 1, 1}, {17, 13, 9}, {64, 64, 64}, {127, 129, 65},
                             {33, 70, 64}, {130, 7, 33}, {128, 128, 1},
                             {96, 132, 36}};
    int failed = 0;
    for (const auto& s : shapes) {
        failed += emuTestShape(name, launchIt, s[0], s[1], s[2]) ? 0 : 1;
    }
    return failed;
}
