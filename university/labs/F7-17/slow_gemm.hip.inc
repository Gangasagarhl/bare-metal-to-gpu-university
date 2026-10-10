// F7-17 forensic evidence: Lin's "MFMA" GEMM kernel as committed (compile only).
// The Makefile builds it with: hipcc -O3 --offload-arch=gfx942 ...
#include <hip/hip_runtime.h>

typedef _Float16 half4 __attribute__((ext_vector_type(4)));
typedef float float16v __attribute__((ext_vector_type(16)));

__global__ void __launch_bounds__(64) linGemm(const _Float16* A, const _Float16* B, float* C,
                                              int M, int N, int K)
{
    const int lane = threadIdx.x;
    const int row0 = blockIdx.y * 32;
    const int col0 = blockIdx.x * 32;
#if defined(__gfx908__) || defined(__gfx90a__) || defined(__gfx940__)
    float16v acc = {};
    for (int k0 = 0; k0 < K; k0 += 8) {
        half4 a;
        half4 b;
        for (int e = 0; e < 4; ++e) {
            a[e] = A[(row0 + lane % 32) * K + k0 + 4 * (lane / 32) + e];
            b[e] = B[(k0 + 4 * (lane / 32) + e) * N + col0 + lane % 32];
        }
        acc = __builtin_amdgcn_mfma_f32_32x32x8f16(a, b, acc, 0, 0, 0);
    }
    for (int e = 0; e < 16; ++e) {
        C[(row0 + 8 * (e / 4) + 4 * (lane / 32) + e % 4) * N + col0 + lane % 32] = acc[e];
    }
#else
    // portable fallback: each lane computes 16 elements of the tile with ordinary FMAs
    for (int e = 0; e < 16; ++e) {
        const int r = row0 + 8 * (e / 4) + 4 * (lane / 32) + e % 4;
        const int c = col0 + lane % 32;
        float acc = 0.0f;
        for (int k = 0; k < K; ++k) {
            acc += static_cast<float>(A[r * K + k]) * static_cast<float>(B[k * N + c]);
        }
        C[r * N + c] = acc;
    }
#endif
}
