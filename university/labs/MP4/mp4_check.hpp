// MP4 Listing 6: one GEMM result against the FP64 reference, with MP3's tolerance policy
// (MP3 Listing 1, mp3_tolerance.hpp, included unchanged): the same rule on every GPU.
#pragma once
#include <cmath>
#include <vector>
#include "../MP3/mp3_tolerance.hpp"

struct Verdict
{
    int bad = 0;            // elements outside the bound (a NaN counts as bad)
    double worst = 0.0;     // largest error / bound seen (1.0 = exactly at the bound)
};

inline Verdict checkGemm(const std::vector<float>& A, const std::vector<float>& B,
                         const std::vector<float>& C, int M, int N, int K)
{
    Verdict v;
    for (int r = 0; r < M; ++r) {
        for (int c = 0; c < N; ++c) {
            double exact = 0.0;
            double sumAbs = 0.0;
            for (int k = 0; k < K; ++k) {
                const double p = static_cast<double>(A[r * K + k]) * B[k * N + c];
                exact += p;
                sumAbs += std::fabs(p);
            }
            const double got = C[r * N + c];
            if (!withinTolerance(got, exact, sumAbs, P_FP32, K)) {
                ++v.bad;
            }
            const double ratio = std::fabs(got - exact) / (relBound(P_FP32, K) * sumAbs + 1e-30);
            if (ratio > v.worst || std::isnan(ratio)) {
                v.worst = ratio;
            }
        }
    }
    return v;
}
