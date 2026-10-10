// F6-22 forensic: a colleague's "tidier" version of sgemmTiled. Spot the difference.
#pragma once

template <int TILE>
__global__ void sgemmTiledTidy(const float* A, const float* B, float* C, int M, int N, int K)
{
    __shared__ float As[TILE][TILE];
    __shared__ float Bs[TILE][TILE];
    const int tx = threadIdx.x;
    const int ty = threadIdx.y;
    const int row = blockIdx.y * TILE + ty;
    const int col = blockIdx.x * TILE + tx;
    if (row >= M || col >= N) {
        return;                                    // "nothing to compute here"
    }
    float acc = 0.0f;
    for (int k0 = 0; k0 < K; k0 += TILE) {
        As[ty][tx] = k0 + tx < K ? A[row * K + k0 + tx] : 0.0f;
        Bs[ty][tx] = k0 + ty < K ? B[(k0 + ty) * N + col] : 0.0f;
        __syncthreads();
        for (int k = 0; k < TILE; ++k) {
            acc += As[ty][k] * Bs[k][tx];
        }
        __syncthreads();
    }
    C[row * N + col] = acc;
}
