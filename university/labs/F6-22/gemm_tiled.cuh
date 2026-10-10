// F6-22 Listing 1: SGEMM with shared-memory tiles. Row-major FP32, C = A * B.
// A block of TILE x TILE threads computes a TILE x TILE tile of C. For each step k0
// along K, the block copies a TILE x TILE tile of A and one of B into shared memory
// (one element per thread), waits, and every thread uses the TILE values of its row
// of As and its column of Bs. Each loaded value is then used TILE times.
#pragma once

template <int TILE>
__global__ void sgemmTiled(const float* A, const float* B, float* C, int M, int N, int K)
{
    __shared__ float As[TILE][TILE];
    __shared__ float Bs[TILE][TILE];
    const int tx = threadIdx.x;                    // column inside the tile
    const int ty = threadIdx.y;                    // row inside the tile
    const int row = blockIdx.y * TILE + ty;
    const int col = blockIdx.x * TILE + tx;
    float acc = 0.0f;
    for (int k0 = 0; k0 < K; k0 += TILE) {
        // every thread loads, even outside C: zeros keep the sums right at the edges
        As[ty][tx] = (row < M && k0 + tx < K) ? A[row * K + k0 + tx] : 0.0f;
        Bs[ty][tx] = (k0 + ty < K && col < N) ? B[(k0 + ty) * N + col] : 0.0f;
        __syncthreads();                           // tiles complete before anyone reads them
        for (int k = 0; k < TILE; ++k) {
            acc += As[ty][k] * Bs[k][tx];
        }
        __syncthreads();                           // everyone done before the tiles are overwritten
    }
    if (row < M && col < N) {
        C[row * N + col] = acc;
    }
}
