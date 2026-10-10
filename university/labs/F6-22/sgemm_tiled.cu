// F6-22 Listing 2: E6 steps 1-2 in the harness (naive, tiled 16, tiled 32). Untested on
// hardware. Build with the cuBLAS column (as run.sh does):
//   nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 -DUSE_CUBLAS sgemm_tiled.cu -lcublas
#include "../F6-21/gemm_harness.cuh"
#include "../F6-21/gemm_naive.cuh"
#include "gemm_tiled.cuh"

void launchNaive(const float* dA, const float* dB, float* dC, int M, int N, int K)
{
    const dim3 block(16, 16);
    sgemmNaive<<<dim3((N + 15) / 16, (M + 15) / 16), block>>>(dA, dB, dC, M, N, K);
}

template <int TILE>
void launchTiled(const float* dA, const float* dB, float* dC, int M, int N, int K)
{
    const dim3 block(TILE, TILE);
    const dim3 grid((N + TILE - 1) / TILE, (M + TILE - 1) / TILE);
    sgemmTiled<TILE><<<grid, block>>>(dA, dB, dC, M, N, K);
}

int main()
{
    return runE6({{"naive", reinterpret_cast<const void*>(sgemmNaive), 256, launchNaive},
                  {"tiled16", reinterpret_cast<const void*>(sgemmTiled<16>), 256, launchTiled<16>},
                  {"tiled32", reinterpret_cast<const void*>(sgemmTiled<32>), 1024, launchTiled<32>}});
}
