// MP4 Listing 12: seeded faults for the CPU emulator. Each is the production kernel with ONE slip.
// If the suite passes a mutant, the suite (shapes or tolerance) is too weak.
#pragma once
#include <algorithm>
#include "gemm_tile.hpp"

// Mutant 1: the loader clamps k to K-1 instead of writing zeros past K (a tempting "simplification").
template <class Cfg>
__global__ void mutantClampK(const float* A, const float* B, float* C, int M, int N, int K)
{
    __shared__ float As[Cfg::BK][Cfg::BM + Cfg::PAD];
    __shared__ float Bs[Cfg::BK][Cfg::BN];
    const int tid = threadIdx.x;
    const int rowBase = blockIdx.y * Cfg::BM;
    const int colBase = blockIdx.x * Cfg::BN;
    float acc[Cfg::TM][Cfg::TN] = {};
    for (int k0 = 0; k0 < K; k0 += Cfg::BK) {
        for (int i = tid; i < Cfg::BM * Cfg::BK; i += Cfg::THREADS) {
            const int gr = rowBase + i / Cfg::BK;
            const int gk = std::min(k0 + i % Cfg::BK, K - 1);           // SLIP: was a zero fill
            As[i % Cfg::BK][i / Cfg::BK] = gr < M ? A[gr * K + gk] : 0.0f;
        }
        for (int i = tid; i < Cfg::BK * Cfg::BN; i += Cfg::THREADS) {
            const int gk = std::min(k0 + i / Cfg::BN, K - 1);           // SLIP
            const int gc = colBase + i % Cfg::BN;
            Bs[i / Cfg::BN][i % Cfg::BN] = gc < N ? B[gk * N + gc] : 0.0f;
        }
        __syncthreads();
        computeStage<Cfg>(tid, As, Bs, acc);
        __syncthreads();
    }
    storeTile<Cfg>(tid, rowBase, colBase, acc, C, M, N);
}

// Mutant 2 (the forensic lab's kernel): the loader copied from F7-16 Listing 1 with its
// literal stride of 256 work-items, correct only for instances that have 256 work-items.
template <class Cfg>
__global__ void mutantStride256(const float* A, const float* B, float* C, int M, int N, int K)
{
    __shared__ float As[Cfg::BK][Cfg::BM + Cfg::PAD];
    __shared__ float Bs[Cfg::BK][Cfg::BN];
    const int tid = threadIdx.x;
    const int rowBase = blockIdx.y * Cfg::BM;
    const int colBase = blockIdx.x * Cfg::BN;
    float acc[Cfg::TM][Cfg::TN] = {};
    for (int k0 = 0; k0 < K; k0 += Cfg::BK) {
        for (int i = tid; i < Cfg::BM * Cfg::BK; i += 256) {          // copied: "4 elements of A per thread"
            const int m = i / Cfg::BK;
            const int k = i % Cfg::BK;
            As[k][m] = (rowBase + m < M && k0 + k < K) ? A[(rowBase + m) * K + k0 + k] : 0.0f;
        }
        for (int i = tid; i < Cfg::BK * Cfg::BN; i += 256) {          // copied: "4 elements of B per thread"
            const int k = i / Cfg::BN;
            const int n = i % Cfg::BN;
            Bs[k][n] = (k0 + k < K && colBase + n < N) ? B[(k0 + k) * N + colBase + n] : 0.0f;
        }
        __syncthreads();
        computeStage<Cfg>(tid, As, Bs, acc);
        __syncthreads();
    }
    storeTile<Cfg>(tid, rowBase, colBase, acc, C, M, N);
}
