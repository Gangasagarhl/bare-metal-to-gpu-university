// F7-17 Listing 1: FP16 x FP16 -> FP32 GEMM on the matrix cores of gfx90a with one MFMA builtin.
// One wave (64 lanes) computes one 32 x 32 tile of C; the K loop advances 8 at a time.
// C = A * B, A is M x K and B is K x N, both row-major FP16; C is row-major FP32.
// M and N must be multiples of 32 and K a multiple of 8 (the host checks this).
// Build: hipcc -std=c++17 -O2 --offload-arch=gfx90a mfma_gemm.hip -o mfma_gemm
#include <hip/hip_runtime.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

typedef _Float16 half4 __attribute__((ext_vector_type(4)));
typedef float float16v __attribute__((ext_vector_type(16)));

// ASSUMED operand layout of v_mfma_f32_32x32x8f16 (NOT VERIFIED in this build: check the
// CDNA2 ISA guide's MFMA chapter or the AMD Matrix Instruction Calculator; the host test
// below fails on hardware if any of these three mappings is wrong).
__device__ int aRow(int lane) { return lane % 32; }              // A element (row, k):
__device__ int aK(int lane, int e) { return 4 * (lane / 32) + e; }   //   lane, element e = 0..3
__device__ int bCol(int lane) { return lane % 32; }              // B element (k, col)
__device__ int bK(int lane, int e) { return 4 * (lane / 32) + e; }
__device__ int cRow(int lane, int e) { return 8 * (e / 4) + 4 * (lane / 32) + e % 4; }  // C: e = 0..15
__device__ int cCol(int lane) { return lane % 32; }

static void check(hipError_t e, const char* what)
{
    if (e != hipSuccess) {
        std::printf("%s failed: %s (%s)\n", what, hipGetErrorName(e), hipGetErrorString(e));
        std::exit(1);
    }
}

__global__ void __launch_bounds__(64) mfmaGemm(const _Float16* A, const _Float16* B, float* C,
                                               int M, int N, int K)
{
    const int lane = threadIdx.x;              // one wave per block
    const int row0 = blockIdx.y * 32;
    const int col0 = blockIdx.x * 32;
    float16v acc = {};
    for (int k0 = 0; k0 < K; k0 += 8) {
        half4 a;
        half4 b;
        for (int e = 0; e < 4; ++e) {
            a[e] = A[(row0 + aRow(lane)) * K + k0 + aK(lane, e)];
            b[e] = B[(k0 + bK(lane, e)) * N + col0 + bCol(lane)];
        }
        // D = A(32x8) * B(8x32) + C(32x32); the last three arguments are cbsz, abid, blgp = 0.
        acc = __builtin_amdgcn_mfma_f32_32x32x8f16(a, b, acc, 0, 0, 0);
    }
    for (int e = 0; e < 16; ++e) {
        C[(row0 + cRow(lane, e)) * N + col0 + cCol(lane)] = acc[e];
    }
}

int main()
{
    const int M = 256, N = 128, K = 520;
    if (M % 32 != 0 || N % 32 != 0 || K % 8 != 0) {
        std::printf("shape not supported\n");
        return 1;
    }
    std::vector<_Float16> hA(static_cast<size_t>(M) * K), hB(static_cast<size_t>(K) * N);
    for (size_t i = 0; i < hA.size(); ++i) {
        hA[i] = static_cast<_Float16>(static_cast<float>(static_cast<int>(i * 37 % 101) - 50) / 64.0f);
    }
    for (size_t i = 0; i < hB.size(); ++i) {
        hB[i] = static_cast<_Float16>(static_cast<float>(static_cast<int>(i * 53 % 89) - 44) / 64.0f);
    }
    _Float16* dA = nullptr;
    _Float16* dB = nullptr;
    float* dC = nullptr;
    check(hipMalloc(&dA, hA.size() * sizeof(_Float16)), "hipMalloc(dA)");
    check(hipMalloc(&dB, hB.size() * sizeof(_Float16)), "hipMalloc(dB)");
    check(hipMalloc(&dC, static_cast<size_t>(M) * N * sizeof(float)), "hipMalloc(dC)");
    check(hipMemcpy(dA, hA.data(), hA.size() * sizeof(_Float16), hipMemcpyHostToDevice), "copy A");
    check(hipMemcpy(dB, hB.data(), hB.size() * sizeof(_Float16), hipMemcpyHostToDevice), "copy B");
    mfmaGemm<<<dim3(N / 32, M / 32), 64>>>(dA, dB, dC, M, N, K);
    check(hipGetLastError(), "launch mfmaGemm");
    check(hipDeviceSynchronize(), "hipDeviceSynchronize");
    std::vector<float> hC(static_cast<size_t>(M) * N);
    check(hipMemcpy(hC.data(), dC, hC.size() * sizeof(float), hipMemcpyDeviceToHost), "copy C");
    // FP16 products are exact in FP32; only the FP32 accumulation rounds: K * 2^-24 * sum|a||b|.
    int bad = 0;
    for (int r = 0; r < M; ++r) {
        for (int c = 0; c < N; ++c) {
            double ref = 0.0, mag = 0.0;
            for (int k = 0; k < K; ++k) {
                const double p = static_cast<double>(hA[r * K + k]) * static_cast<double>(hB[k * N + c]);
                ref += p;
                mag += std::fabs(p);
            }
            if (std::fabs(hC[r * N + c] - ref) > K * std::ldexp(1.0, -24) * mag) {
                if (bad < 5) {
                    std::printf("C[%d][%d] = %g, expected %g\n", r, c, static_cast<double>(hC[r * N + c]), ref);
                }
                ++bad;
            }
        }
    }
    std::printf("M=%d N=%d K=%d: %d elements outside tolerance\n", M, N, K, bad);
    check(hipFree(dA), "hipFree");
    check(hipFree(dB), "hipFree");
    check(hipFree(dC), "hipFree");
    return bad == 0 ? 0 : 1;
}
