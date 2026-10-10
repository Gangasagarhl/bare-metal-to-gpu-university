// F7-18 Listing 2: a list of instances, each asked whether it supports a shape, then checked.
#include "mini_ck.hpp"
#include <cmath>
#include <cstdio>
#include <tuple>

template <class P>
void tryInstance(const std::vector<float>& A, const std::vector<float>& B, int M, int N, int K)
{
    if (!P::isSupported(M, N, K)) {
        std::printf("  %-26s threads %4d  LDS %6d B  not supported\n", P::name().c_str(), P::threads,
                    P::ldsBytes);
        return;
    }
    std::vector<float> C(static_cast<std::size_t>(M) * N, 0.0f);
    P::run(A, B, C, M, N, K);
    int bad = 0;
    for (int r = 0; r < M; ++r) {
        for (int c = 0; c < N; ++c) {
            double ref = 0, mag = 0;
            for (int k = 0; k < K; ++k) {
                const double p = static_cast<double>(A[static_cast<std::size_t>(r) * K + k]) *
                                 B[static_cast<std::size_t>(k) * N + c];
                ref += p;
                mag += std::fabs(p);
            }
            bad += std::fabs(C[static_cast<std::size_t>(r) * N + c] - ref) > 2.0 * K * std::ldexp(1.0, -24) * mag;
        }
    }
    const long long wgs = static_cast<long long>((M + P::BM - 1) / P::BM) * ((N + P::BN - 1) / P::BN);
    const double used = static_cast<double>(M) * N / (wgs * P::BM * P::BN);
    std::printf("  %-26s threads %4d  LDS %6d B  WGs %5lld  tile use %5.1f %%  wrong %d\n",
                P::name().c_str(), P::threads, P::ldsBytes, wgs, 100.0 * used, bad);
}

int main()
{
    using I0 = mini::GemmPolicy<128, 128, 16, 8, 8, false>;
    using I1 = mini::GemmPolicy<64, 64, 16, 4, 4, false>;
    using I2 = mini::GemmPolicy<64, 64, 16, 4, 4, true>;
    using I3 = mini::GemmPolicy<32, 64, 32, 2, 4, true>;
    struct Shape { int M, N, K; };
    const Shape shapes[] = {{256, 256, 256}, {200, 72, 48}, {96, 96, 40}};
    for (const Shape& s : shapes) {
        std::vector<float> A(static_cast<std::size_t>(s.M) * s.K), B(static_cast<std::size_t>(s.K) * s.N);
        for (std::size_t i = 0; i < A.size(); ++i) A[i] = static_cast<float>(static_cast<int>(i * 37 % 101) - 50) / 50.0f;
        for (std::size_t i = 0; i < B.size(); ++i) B[i] = static_cast<float>(static_cast<int>(i * 53 % 89) - 44) / 44.0f;
        std::printf("M=%d N=%d K=%d\n", s.M, s.N, s.K);
        tryInstance<I0>(A, B, s.M, s.N, s.K);
        tryInstance<I1>(A, B, s.M, s.N, s.K);
        tryInstance<I2>(A, B, s.M, s.N, s.K);
        tryInstance<I3>(A, B, s.M, s.N, s.K);
    }
    return 0;
}
