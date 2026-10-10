// F6-23 Listing 1: SGEMM with a block tile in shared memory and a thread tile in registers.
// Row-major FP32, C = A * B. A block of (BM/TM)*(BN/TN) threads computes a BM x BN tile of
// C; each thread keeps a TM x TN tile of C in registers (acc). Per step k it reads TM values
// of A and TN values of B from shared memory and does TM*TN multiply-adds with them.
// VEC = true loads A, B and the shared tiles with 16-byte float4 instructions; it needs
// K and N to be multiples of 4 (the launcher checks) and TM, TN multiples of 4.
// MINB is the "minimum blocks per SM" hint of __launch_bounds__ (1 = no pressure).
#pragma once

template <int BM, int BN, int BK, int TM, int TN, bool VEC, int MINB = 1>
__global__ void __launch_bounds__((BM / TM) * (BN / TN), MINB)
sgemmRegTile(const float* A, const float* B, float* C, int M, int N, int K)
{
    constexpr int THREADS = (BM / TM) * (BN / TN);
    alignas(16) __shared__ float As[BK][BM];       // A tile stored transposed: As[k][m]
    alignas(16) __shared__ float Bs[BK][BN];
    const int tid = threadIdx.x;
    const int tRow = tid / (BN / TN);              // which group of TM rows this thread owns
    const int tCol = tid % (BN / TN);              // which group of TN columns
    const int rowBase = blockIdx.y * BM;
    const int colBase = blockIdx.x * BN;
    float acc[TM][TN] = {};
    float a[TM];
    float b[TN];
    for (int k0 = 0; k0 < K; k0 += BK) {
        if constexpr (VEC) {
            for (int i = tid; i < BM * BK / 4; i += THREADS) {   // A: 4 consecutive k per load
                const int m = i / (BK / 4), k = (i % (BK / 4)) * 4;
                const int r = rowBase + m, c = k0 + k;
                float4 v = {0.0f, 0.0f, 0.0f, 0.0f};
                if (r < M && c < K) {
                    v = *reinterpret_cast<const float4*>(&A[r * K + c]);
                }
                As[k][m] = v.x;
                As[k + 1][m] = v.y;
                As[k + 2][m] = v.z;
                As[k + 3][m] = v.w;
            }
            for (int i = tid; i < BK * BN / 4; i += THREADS) {   // B: 4 consecutive n per load
                const int k = i / (BN / 4), n = (i % (BN / 4)) * 4;
                const int r = k0 + k, c = colBase + n;
                float4 v = {0.0f, 0.0f, 0.0f, 0.0f};
                if (r < K && c < N) {
                    v = *reinterpret_cast<const float4*>(&B[r * N + c]);
                }
                *reinterpret_cast<float4*>(&Bs[k][n]) = v;
            }
        } else {
            for (int i = tid; i < BM * BK; i += THREADS) {
                const int m = i / BK, k = i % BK;
                const int r = rowBase + m, c = k0 + k;
                As[k][m] = (r < M && c < K) ? A[r * K + c] : 0.0f;
            }
            for (int i = tid; i < BK * BN; i += THREADS) {
                const int k = i / BN, n = i % BN;
                const int r = k0 + k, c = colBase + n;
                Bs[k][n] = (r < K && c < N) ? B[r * N + c] : 0.0f;
            }
        }
        __syncthreads();
#pragma unroll
        for (int k = 0; k < BK; ++k) {
            if constexpr (VEC) {
#pragma unroll
                for (int i = 0; i < TM; i += 4) {
                    const float4 v = *reinterpret_cast<const float4*>(&As[k][tRow * TM + i]);
                    a[i] = v.x; a[i + 1] = v.y; a[i + 2] = v.z; a[i + 3] = v.w;
                }
#pragma unroll
                for (int j = 0; j < TN; j += 4) {
                    const float4 v = *reinterpret_cast<const float4*>(&Bs[k][tCol * TN + j]);
                    b[j] = v.x; b[j + 1] = v.y; b[j + 2] = v.z; b[j + 3] = v.w;
                }
            } else {
#pragma unroll
                for (int i = 0; i < TM; ++i) {
                    a[i] = As[k][tRow * TM + i];
                }
#pragma unroll
                for (int j = 0; j < TN; ++j) {
                    b[j] = Bs[k][tCol * TN + j];
                }
            }
#pragma unroll
            for (int i = 0; i < TM; ++i) {
#pragma unroll
                for (int j = 0; j < TN; ++j) {
                    acc[i][j] += a[i] * b[j];      // TM*TN multiply-adds for TM+TN shared loads
                }
            }
        }
        __syncthreads();
    }
#pragma unroll
    for (int i = 0; i < TM; ++i) {
#pragma unroll
        for (int j = 0; j < TN; ++j) {
            const int r = rowBase + tRow * TM + i, c = colBase + tCol * TN + j;
            if (r < M && c < N) {
                C[r * N + c] = acc[i][j];
            }
        }
    }
}
