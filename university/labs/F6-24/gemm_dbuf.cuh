// F6-24 Listing 1: register-tiled SGEMM with two shared-memory stages filled by
// asynchronous copies (cp.async through the cuda_pipeline primitives). While the block
// computes on stage t % 2, the copies for tile t + 1 are already on their way into the
// other stage. Row-major FP32, C = A * B. Needs K and N to be multiples of 4 (16-byte
// copies); the launcher falls back to the F6-23 kernel otherwise.
#pragma once

template <int BM, int BN, int BK, int TM, int TN>
__global__ void sgemmDoubleBuffer(const float* A, const float* B, float* C, int M, int N, int K)
{
    constexpr int THREADS = (BM / TM) * (BN / TN);
    alignas(16) __shared__ float As[2][BM][BK];     // copied as is: 4 consecutive k per copy
    alignas(16) __shared__ float Bs[2][BK][BN];
    const int tid = threadIdx.x;
    const int tRow = tid / (BN / TN);
    const int tCol = tid % (BN / TN);
    const int rowBase = blockIdx.y * BM;
    const int colBase = blockIdx.x * BN;

    auto loadTile = [&](int stage, int k0) {        // issue the copies; do not wait
        for (int i = tid; i < BM * BK / 4; i += THREADS) {
            const int m = i / (BK / 4), k = (i % (BK / 4)) * 4;
            const int r = rowBase + m, c = k0 + k;
            if (r < M && c < K) {
                __pipeline_memcpy_async(&As[stage][m][k], &A[r * K + c], 16);
            } else {
                As[stage][m][k] = As[stage][m][k + 1] = As[stage][m][k + 2] = As[stage][m][k + 3] = 0.0f;
            }
        }
        for (int i = tid; i < BK * BN / 4; i += THREADS) {
            const int k = i / (BN / 4), n = (i % (BN / 4)) * 4;
            const int r = k0 + k, c = colBase + n;
            if (r < K && c < N) {
                __pipeline_memcpy_async(&Bs[stage][k][n], &B[r * N + c], 16);
            } else {
                Bs[stage][k][n] = Bs[stage][k][n + 1] = Bs[stage][k][n + 2] = Bs[stage][k][n + 3] = 0.0f;
            }
        }
    };

    float acc[TM][TN] = {};
    const int tiles = (K + BK - 1) / BK;
    loadTile(0, 0);
    __pipeline_commit();                            // group for tile 0
    for (int t = 0; t < tiles; ++t) {
        if (t + 1 < tiles) {
            loadTile((t + 1) % 2, (t + 1) * BK);    // prefetch the next tile into the other stage
        }
        __pipeline_commit();                        // group for tile t + 1 (may be empty)
        __pipeline_wait_prior(1);                   // my copies for tile t have landed ...
        __syncthreads();                            // ... and everybody else's too
        const int s = t % 2;
#pragma unroll
        for (int k = 0; k < BK; ++k) {
            float a[TM], b[TN];
#pragma unroll
            for (int i = 0; i < TM; ++i) {
                a[i] = As[s][tRow * TM + i][k];
            }
#pragma unroll
            for (int j = 0; j < TN; ++j) {
                b[j] = Bs[s][k][tCol * TN + j];
            }
#pragma unroll
            for (int i = 0; i < TM; ++i) {
#pragma unroll
                for (int j = 0; j < TN; ++j) {
                    acc[i][j] += a[i] * b[j];
                }
            }
        }
        __syncthreads();                            // stage s is free before it is refilled
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

template <int BM, int BN, int BK, int TM, int TN>
void launchDoubleBuffer(const float* A, const float* B, float* C, int M, int N, int K)
{
    const dim3 block((BM / TM) * (BN / TN));
    const dim3 grid((N + BN - 1) / BN, (M + BM - 1) / BM);
    if (K % 4 != 0 || N % 4 != 0) {
        LAUNCH((sgemmRegTile<BM, BN, BK, TM, TN, false>), grid, block, A, B, C, M, N, K);
    } else {
        LAUNCH((sgemmDoubleBuffer<BM, BN, BK, TM, TN>), grid, block, A, B, C, M, N, K);
    }
}
