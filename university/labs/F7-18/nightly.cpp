// F7-18 forensic evidence: the nightly test of Ana's dispatcher, which always takes the
// fastest-looking instance (largest tile) and does not ask it whether it supports the shape.
#include "mini_ck.hpp"
#include <cmath>
#include <cstdio>

int main()
{
    using Fast = mini::GemmPolicy<128, 128, 16, 8, 8, false>;
    const int shapes[][3] = {{512, 512, 512}, {256, 256, 64}, {256, 256, 72}, {130, 250, 96},
                             {128, 128, 100}, {300, 200, 33}, {64, 64, 16}, {64, 64, 24}};
    int failed = 0;
    for (const auto& s : shapes) {
        const int M = s[0], N = s[1], K = s[2];
        std::vector<float> A(static_cast<std::size_t>(M) * K), B(static_cast<std::size_t>(K) * N);
        for (std::size_t i = 0; i < A.size(); ++i) A[i] = static_cast<float>(static_cast<int>(i * 37 % 101) - 50) / 50.0f;
        for (std::size_t i = 0; i < B.size(); ++i) B[i] = static_cast<float>(static_cast<int>(i * 53 % 89) - 44) / 44.0f;
        std::vector<float> C(static_cast<std::size_t>(M) * N, 0.0f);
        Fast::run(A, B, C, M, N, K);
        int bad = 0;
        double worst = 0;
        for (int r = 0; r < M; ++r) {
            for (int c = 0; c < N; ++c) {
                double ref = 0, mag = 0;
                for (int k = 0; k < K; ++k) {
                    const double p = static_cast<double>(A[static_cast<std::size_t>(r) * K + k]) * B[static_cast<std::size_t>(k) * N + c];
                    ref += p;
                    mag += std::fabs(p);
                }
                const double err = std::fabs(C[static_cast<std::size_t>(r) * N + c] - ref);
                if (err > 2.0 * K * std::ldexp(1.0, -24) * mag) {
                    ++bad;
                    worst = std::fmax(worst, err / mag);
                }
            }
        }
        failed += bad > 0;
        std::printf("test M=%-4d N=%-4d K=%-4d %s", M, N, K, bad == 0 ? "PASS" : "FAIL");
        if (bad > 0) {
            std::printf("  wrong %d of %d, worst relative error %.3f", bad, M * N, worst);
        }
        std::printf("\n");
    }
    std::printf("%d of 8 tests failed\n", failed);
    return 0;
}
