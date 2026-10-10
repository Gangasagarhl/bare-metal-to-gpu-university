// F7-16 Listing 1: three SGEMM kernels, C = A * B, row-major, any M, N, K.
//   sgemmNaive    - one thread per element of C, everything from global memory
//   sgemmLds      - 16 x 16 tiles of A and B staged in LDS, one output per thread
//   sgemmLdsReg   - 64 x 64 block tile, BK = 16, each thread keeps a 4 x 4 tile of C in registers
// Build: hipcc -std=c++17 -O2 --offload-arch=gfx90a sgemm_lds.hip -o sgemm_lds
#include <hip/hip_runtime.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(hipError_t e, const char* what)
{
    if (e != hipSuccess) {
        std::printf("%s failed: %s (%s)\n", what, hipGetErrorName(e), hipGetErrorString(e));
        std::exit(1);
    }
}

__global__ void sgemmNaive(const float* A, const float* B, float* C, int M, int N, int K)
{
    const int row = blockIdx.y * blockDim.y + threadIdx.y;
    const int col = blockIdx.x * blockDim.x + threadIdx.x;
    if (row < M && col < N) {
        float acc = 0.0f;
        for (int k = 0; k < K; ++k) {
            acc += A[row * K + k] * B[k * N + col];
        }
        C[row * N + col] = acc;
    }
}

constexpr int T = 16;
__global__ void sgemmLds(const float* A, const float* B, float* C, int M, int N, int K)
{
    __shared__ float As[T][T];
    __shared__ float Bs[T][T];
    const int tx = threadIdx.x;
    const int ty = threadIdx.y;
    const int row = blockIdx.y * T + ty;
    const int col = blockIdx.x * T + tx;
    float acc = 0.0f;
    for (int k0 = 0; k0 < K; k0 += T) {
        As[ty][tx] = (row < M && k0 + tx < K) ? A[row * K + k0 + tx] : 0.0f;
        Bs[ty][tx] = (k0 + ty < K && col < N) ? B[(k0 + ty) * N + col] : 0.0f;
        __syncthreads();                       // tile complete before anyone reads it
        for (int k = 0; k < T; ++k) {
            acc += As[ty][k] * Bs[k][tx];
        }
        __syncthreads();                       // everyone done before the tile is overwritten
    }
    if (row < M && col < N) {
        C[row * N + col] = acc;
    }
}

constexpr int BM = 64;
constexpr int BN = 64;
constexpr int BK = 16;
constexpr int TM = 4;
constexpr int TN = 4;
constexpr int PAD = 4;                         // keeps rows of As 16-byte aligned, shifts banks
__global__ void __launch_bounds__(256) sgemmLdsReg(const float* A, const float* B, float* C,
                                                   int M, int N, int K)
{
    __shared__ float As[BK][BM + PAD];         // A tile stored transposed: As[k][m]
    __shared__ float Bs[BK][BN];
    const int tid = threadIdx.x;               // 256 threads = 16 x 16
    const int tRow = tid / 16;                 // this thread owns rows tRow*4 .. tRow*4+3
    const int tCol = tid % 16;                 // and columns tCol*4 .. tCol*4+3
    const int rowBase = blockIdx.y * BM;
    const int colBase = blockIdx.x * BN;
    float acc[TM][TN] = {};
    for (int k0 = 0; k0 < K; k0 += BK) {
        for (int i = tid; i < BM * BK; i += 256) {   // 4 elements of A per thread
            const int m = i / BK;
            const int k = i % BK;
            const int gr = rowBase + m;
            const int gk = k0 + k;
            As[k][m] = (gr < M && gk < K) ? A[gr * K + gk] : 0.0f;
        }
        for (int i = tid; i < BK * BN; i += 256) {   // 4 elements of B per thread
            const int k = i / BN;
            const int n = i % BN;
            const int gk = k0 + k;
            const int gc = colBase + n;
            Bs[k][n] = (gk < K && gc < N) ? B[gk * N + gc] : 0.0f;
        }
        __syncthreads();
        for (int k = 0; k < BK; ++k) {
            float a[TM];
            float b[TN];
            for (int i = 0; i < TM; ++i) {
                a[i] = As[k][tRow * TM + i];
            }
            for (int j = 0; j < TN; ++j) {
                b[j] = Bs[k][tCol * TN + j];
            }
            for (int i = 0; i < TM; ++i) {
                for (int j = 0; j < TN; ++j) {
                    acc[i][j] += a[i] * b[j];     // 16 FMAs for 8 LDS reads
                }
            }
        }
        __syncthreads();
    }
    for (int i = 0; i < TM; ++i) {
        for (int j = 0; j < TN; ++j) {
            const int r = rowBase + tRow * TM + i;
            const int c = colBase + tCol * TN + j;
            if (r < M && c < N) {
                C[r * N + c] = acc[i][j];
            }
        }
    }
}

struct Shape { int M; int N; int K; const char* what; };

int main()
{
    const Shape shapes[] = {
        {512, 512, 512, "square"},
        {1000, 1000, 1000, "square, not a tile multiple"},
        {4096, 64, 256, "tall-skinny"},
        {64, 4096, 256, "short-wide"},
        {77, 129, 33, "odd sizes"},
    };
    for (const Shape& s : shapes) {
        const int M = s.M, N = s.N, K = s.K;
        std::vector<float> hA(static_cast<size_t>(M) * K), hB(static_cast<size_t>(K) * N);
        for (size_t i = 0; i < hA.size(); ++i) {
            hA[i] = static_cast<float>(static_cast<int>(i * 37 % 101) - 50) / 50.0f;
        }
        for (size_t i = 0; i < hB.size(); ++i) {
            hB[i] = static_cast<float>(static_cast<int>(i * 53 % 89) - 44) / 44.0f;
        }
        float *dA = nullptr, *dB = nullptr, *dC = nullptr;
        check(hipMalloc(&dA, hA.size() * sizeof(float)), "hipMalloc(dA)");
        check(hipMalloc(&dB, hB.size() * sizeof(float)), "hipMalloc(dB)");
        check(hipMalloc(&dC, static_cast<size_t>(M) * N * sizeof(float)), "hipMalloc(dC)");
        check(hipMemcpy(dA, hA.data(), hA.size() * sizeof(float), hipMemcpyHostToDevice), "copy A");
        check(hipMemcpy(dB, hB.data(), hB.size() * sizeof(float), hipMemcpyHostToDevice), "copy B");
        std::vector<float> hC(static_cast<size_t>(M) * N);
        for (int kernel = 0; kernel < 3; ++kernel) {
            check(hipMemset(dC, 0, hC.size() * sizeof(float)), "hipMemset");
            if (kernel == 0) {
                sgemmNaive<<<dim3((N + 15) / 16, (M + 15) / 16), dim3(16, 16)>>>(dA, dB, dC, M, N, K);
            } else if (kernel == 1) {
                sgemmLds<<<dim3((N + T - 1) / T, (M + T - 1) / T), dim3(T, T)>>>(dA, dB, dC, M, N, K);
            } else {
                sgemmLdsReg<<<dim3((N + BN - 1) / BN, (M + BM - 1) / BM), 256>>>(dA, dB, dC, M, N, K);
            }
            check(hipGetLastError(), "launch");
            check(hipDeviceSynchronize(), "hipDeviceSynchronize");
            check(hipMemcpy(hC.data(), dC, hC.size() * sizeof(float), hipMemcpyDeviceToHost), "copy C");
            // Tolerance per element: 2 * K * u * sum_k |a_ik| |b_kj|, u = 2^-24 (FP32 rounding).
            int bad = 0;
            for (int r = 0; r < M; ++r) {
                for (int c = 0; c < N; ++c) {
                    double ref = 0.0, mag = 0.0;
                    for (int k = 0; k < K; ++k) {
                        const double p = static_cast<double>(hA[r * K + k]) * hB[k * N + c];
                        ref += p;
                        mag += std::fabs(p);
                    }
                    const double tol = 2.0 * K * std::ldexp(1.0, -24) * mag;
                    if (std::fabs(hC[r * N + c] - ref) > tol) {
                        ++bad;
                    }
                }
            }
            std::printf("%-28s M=%-5d N=%-5d K=%-5d kernel %d: %d elements outside tolerance\n",
                        s.what, M, N, K, kernel, bad);
        }
        check(hipFree(dA), "hipFree");
        check(hipFree(dB), "hipFree");
        check(hipFree(dC), "hipFree");
    }
    return 0;
}
