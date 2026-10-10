// F7-16 forensic evidence: Kai's register-tiled SGEMM kernel, as committed (compile only; built with -x hip).
#include <hip/hip_runtime.h>

constexpr int BM = 64;
constexpr int BN = 64;
constexpr int BK = 16;
constexpr int TM = 4;
constexpr int TN = 4;
constexpr int PAD = 4;                         // keeps rows of As 16-byte aligned, shifts banks
__global__ void __launch_bounds__(256) kaiGemm(const float* A, const float* B, float* C,
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

