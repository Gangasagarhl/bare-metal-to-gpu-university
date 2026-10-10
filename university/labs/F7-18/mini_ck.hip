// F7-18 Listing 4: the same policies drive a real HIP kernel template; three instances are compiled.
// Build: hipcc -std=c++17 -O2 --offload-arch=gfx90a mini_ck.hip -o mini_ck
#include <hip/hip_runtime.h>
#include "mini_ck.hpp"
#include <cstdio>
#include <cstdlib>

template <class P>
__global__ void __launch_bounds__(P::threads) tileGemm(const float* A, const float* B, float* C,
                                                       int M, int N, int K)
{
    __shared__ float As[P::BK][P::BM];
    __shared__ float Bs[P::BK][P::BN];
    const int tid = threadIdx.x;
    const int tRow = tid / (P::BN / P::TN);
    const int tCol = tid % (P::BN / P::TN);
    const int r0 = blockIdx.y * P::BM;
    const int c0 = blockIdx.x * P::BN;
    float acc[P::TM][P::TN] = {};
    for (int k0 = 0; k0 < K; k0 += P::BK) {
        for (int i = tid; i < P::BM * P::BK; i += P::threads) {
            const int m = i / P::BK, k = i % P::BK;
            const bool ok = r0 + m < M && (!P::PadK || k0 + k < K);
            As[k][m] = ok ? A[(r0 + m) * K + k0 + k] : 0.0f;
        }
        for (int i = tid; i < P::BK * P::BN; i += P::threads) {
            const int k = i / P::BN, n = i % P::BN;
            const bool ok = c0 + n < N && (!P::PadK || k0 + k < K);
            Bs[k][n] = ok ? B[(k0 + k) * N + c0 + n] : 0.0f;
        }
        __syncthreads();
        for (int k = 0; k < P::BK; ++k) {
            for (int i = 0; i < P::TM; ++i) {
                for (int j = 0; j < P::TN; ++j) {
                    acc[i][j] += As[k][tRow * P::TM + i] * Bs[k][tCol * P::TN + j];
                }
            }
        }
        __syncthreads();
    }
    for (int i = 0; i < P::TM; ++i) {
        for (int j = 0; j < P::TN; ++j) {
            const int r = r0 + tRow * P::TM + i, c = c0 + tCol * P::TN + j;
            if (r < M && c < N) {
                C[r * N + c] = acc[i][j];
            }
        }
    }
}

template <class P>
hipError_t launch(const float* dA, const float* dB, float* dC, int M, int N, int K)
{
    if (!P::isSupported(M, N, K)) {
        return hipErrorInvalidValue;            // the dispatcher must try another instance
    }
    const dim3 grid((N + P::BN - 1) / P::BN, (M + P::BM - 1) / P::BM);
    tileGemm<P><<<grid, P::threads>>>(dA, dB, dC, M, N, K);
    return hipGetLastError();
}

using I0 = mini::GemmPolicy<128, 128, 16, 8, 8, false>;
using I1 = mini::GemmPolicy<64, 64, 16, 4, 4, true>;
using I2 = mini::GemmPolicy<32, 64, 32, 2, 4, true>;

int main()
{
    const int M = 96, N = 96, K = 40;
    float *dA = nullptr, *dB = nullptr, *dC = nullptr;
    hipError_t e = hipMalloc(&dA, M * K * sizeof(float));
    if (e != hipSuccess) {
        std::printf("hipMalloc failed: %s (%s)\n", hipGetErrorName(e), hipGetErrorString(e));
        return 1;
    }
    if (hipMalloc(&dB, K * N * sizeof(float)) != hipSuccess || hipMalloc(&dC, M * N * sizeof(float)) != hipSuccess) {
        std::printf("hipMalloc failed\n");
        return 1;
    }
    // try the instances in order of preference, as a library dispatcher does
    const char* used = "none";
    if (launch<I0>(dA, dB, dC, M, N, K) == hipSuccess) {
        used = "I0";
    } else if (launch<I1>(dA, dB, dC, M, N, K) == hipSuccess) {
        used = "I1";
    } else if (launch<I2>(dA, dB, dC, M, N, K) == hipSuccess) {
        used = "I2";
    }
    std::printf("instance used: %s; sync: %s\n", used, hipGetErrorString(hipDeviceSynchronize()));
    (void)hipFree(dA);
    (void)hipFree(dB);
    (void)hipFree(dC);
    return 0;
}
