// MP4 Listing 5: from a run-time instance index to a compile-time kernel instance.
// LAUNCH is defined by the includer: kernel<<<grid, block>>>(...) on a GPU, launch(...) in the
// CPU emulator (the F6-23 pattern), so this file is shared by the GPU program and the CPU suite.
#pragma once
#include "sgemm_kernel.hpp"
#include "tuning.hpp"

template <class Cfg>
void launchCfg(const float* A, const float* B, float* C, int M, int N, int K)
{
    const dim3 grid((N + Cfg::BN - 1) / Cfg::BN, (M + Cfg::BM - 1) / Cfg::BM);
    LAUNCH(sgemmTiled<Cfg>, grid, dim3(Cfg::THREADS), A, B, C, M, N, K);
}

// Adding an instance to kInstances (tuning.hpp) means adding one case here.
inline bool launchInstance(int idx, const float* A, const float* B, float* C, int M, int N, int K)
{
    switch (idx) {
    case 0: launchCfg<Inst0>(A, B, C, M, N, K); return true;
    case 1: launchCfg<Inst1>(A, B, C, M, N, K); return true;
    case 2: launchCfg<Inst2>(A, B, C, M, N, K); return true;
    default: return false;
    }
}
