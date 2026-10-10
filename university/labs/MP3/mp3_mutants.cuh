// MP3: three seeded faults ("mutants") that the correctness suite MUST catch.
// Each is a realistic one-line slip in a correct CU302 kernel. If the suite passes a mutant,
// the suite (its shapes or its tolerance) is too weak. Emulator only: mutant 3 calls the
// host rounding model of F6-25, so this file is not built for the GPU.
#pragma once
#include "../F6-25/lowp.hpp"

// Mutant 1 (from F6-22's sgemmTiled): the K loop stops before a partial last tile.
// Correct whenever K is a multiple of TILE, so square power-of-two tests never see it.
template <int TILE>
__global__ void mutantSkipTail(const float* A, const float* B, float* C, int M, int N, int K)
{
    __shared__ float As[TILE][TILE];
    __shared__ float Bs[TILE][TILE];
    const int tx = threadIdx.x, ty = threadIdx.y;
    const int row = blockIdx.y * TILE + ty, col = blockIdx.x * TILE + tx;
    float acc = 0.0f;
    for (int k0 = 0; k0 + TILE <= K; k0 += TILE) {          // SLIP: was k0 < K
        As[ty][tx] = (row < M) ? A[row * K + k0 + tx] : 0.0f;
        Bs[ty][tx] = (col < N) ? B[(k0 + ty) * N + col] : 0.0f;
        __syncthreads();
        for (int k = 0; k < TILE; ++k) {
            acc += As[ty][k] * Bs[k][tx];
        }
        __syncthreads();
    }
    if (row < M && col < N) {
        C[row * N + col] = acc;
    }
}

// Mutant 2 is a launcher slip, not a kernel slip: see launchFloorGrid in mp3_suite.cpp.

// Mutant 3 (from F6-21's sgemmNaive): the running sum is kept in BF16 instead of FP32,
// as when an accumulator type is chosen wrongly in a mixed-precision kernel. Small K hides it.
__global__ void mutantBf16Acc(const float* A, const float* B, float* C, int M, int N, int K)
{
    const int col = blockIdx.x * blockDim.x + threadIdx.x;
    const int row = blockIdx.y * blockDim.y + threadIdx.y;
    if (row < M && col < N) {
        float acc = 0.0f;
        for (int k = 0; k < K; ++k) {
            acc = static_cast<float>(roundTo(acc + A[row * K + k] * B[k * N + col], BF16));  // SLIP
        }
        C[row * N + col] = acc;
    }
}
