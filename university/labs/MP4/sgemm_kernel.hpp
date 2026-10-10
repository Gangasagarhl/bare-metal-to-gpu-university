// MP4 Listing 3: the SGEMM kernel template. The same text is compiled three ways:
//   hipcc for AMD targets, nvcc with HIP's NVIDIA headers, and g++ with the university's
//   CPU emulator (F6-21 cuda_shim.hpp), which runs each workgroup as real threads and a real barrier.
// The includer provides the GPU dialect first (hip_runtime.h or cuda_shim.hpp).
#pragma once
#include "gemm_tile.hpp"

template <class Cfg>
__global__ void __launch_bounds__(Cfg::THREADS)
sgemmTiled(const float* A, const float* B, float* C, int M, int N, int K)
{
    __shared__ float As[Cfg::BK][Cfg::BM + Cfg::PAD];
    __shared__ float Bs[Cfg::BK][Cfg::BN];
    const int tid = threadIdx.x;
    const int rowBase = blockIdx.y * Cfg::BM;
    const int colBase = blockIdx.x * Cfg::BN;
    float acc[Cfg::TM][Cfg::TN] = {};
    for (int k0 = 0; k0 < K; k0 += Cfg::BK) {
        loadStage<Cfg>(tid, k0, rowBase, colBase, A, B, M, N, K, As, Bs);
        __syncthreads();                       // stage complete before anyone reads it
        computeStage<Cfg>(tid, As, Bs, acc);
        __syncthreads();                       // everyone done before the stage is overwritten
    }
    storeTile<Cfg>(tid, rowBase, colBase, acc, C, M, N);
}
