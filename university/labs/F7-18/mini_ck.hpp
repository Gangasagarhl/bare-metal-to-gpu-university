// F7-18 Listing 1: a miniature "instance" library in the style of a tile-based kernel library.
// One template describes a whole family of GEMM kernels; each set of parameters is an instance.
// Compile-time checks reject impossible instances; a run-time check rejects unsupported shapes.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace mini {

template <int BM_, int BN_, int BK_, int TM_, int TN_, bool PadK_>
struct GemmPolicy
{
    static constexpr int BM = BM_, BN = BN_, BK = BK_;     // block tile (one workgroup)
    static constexpr int TM = TM_, TN = TN_;               // thread tile (registers)
    static constexpr bool PadK = PadK_;                    // handles K % BK != 0 by zero-filling
    static constexpr int threads = (BM / TM) * (BN / TN);
    static constexpr int ldsBytes = (BK * BM + BK * BN) * 4;
    static_assert(BM % TM == 0 && BN % TN == 0, "thread tile must divide the block tile");
    static_assert(threads % 64 == 0, "workgroup must be a whole number of 64-lane waves");
    static_assert(threads <= 1024, "too many threads per workgroup");
    static_assert(ldsBytes <= 65536, "LDS tile larger than the per-workgroup limit");

    static std::string name()
    {
        return "Gemm<" + std::to_string(BM) + "x" + std::to_string(BN) + "x" + std::to_string(BK) +
               ", " + std::to_string(TM) + "x" + std::to_string(TN) + (PadK ? ", PadK>" : ">");
    }
    // Like a library's argument check: the instance tells you whether it can run this shape.
    static bool isSupported(int M, int N, int K)
    {
        (void)M;
        (void)N;
        return PadK || K % BK == 0;
    }
    // Host replay of the kernel's algorithm (block tile by block tile, k-tile by k-tile).
    // M and N edges are always guarded; K is guarded only when PadK is true.
    static void run(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C,
                    int M, int N, int K)
    {
        for (int r0 = 0; r0 < M; r0 += BM) {
            for (int c0 = 0; c0 < N; c0 += BN) {
                std::vector<float> acc(static_cast<std::size_t>(BM) * BN, 0.0f);
                for (int k0 = 0; k0 < K; k0 += BK) {
                    for (int k = 0; k < BK; ++k) {
                        const int gk = k0 + k;
                        const bool kOk = gk < K;
                        for (int m = 0; m < BM; ++m) {
                            for (int n = 0; n < BN; ++n) {
                                const int gr = r0 + m, gc = c0 + n;
                                if (gr >= M || gc >= N) {
                                    continue;
                                }
                                if (PadK && !kOk) {
                                    continue;              // zero-filled in LDS
                                }
                                // an unpadded instance on a bad K reads past the end of a row of A
                                // (into the next row) and past the last row of B; the replay wraps
                                // such reads around to the start of the array, standing in for
                                // whatever memory follows it on a GPU
                                const std::size_t ia = (static_cast<std::size_t>(gr) * K + gk) % A.size();
                                const std::size_t ib = (static_cast<std::size_t>(gk) * N + gc) % B.size();
                                const float a = A[ia];
                                const float b = B[ib];
                                acc[static_cast<std::size_t>(m) * BN + n] += a * b;
                            }
                        }
                    }
                }
                for (int m = 0; m < BM && r0 + m < M; ++m) {
                    for (int n = 0; n < BN && c0 + n < N; ++n) {
                        C[static_cast<std::size_t>(r0 + m) * N + c0 + n] = acc[static_cast<std::size_t>(m) * BN + n];
                    }
                }
            }
        }
    }
};

}  // namespace mini
