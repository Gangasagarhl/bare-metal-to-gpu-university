// F6-21 Listing 2: E6 step 1, the naive kernel in the harness. Untested on hardware.
// Build with the cuBLAS column (as run.sh does):
//   nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 -DUSE_CUBLAS sgemm_naive.cu -lcublas
#include "gemm_harness.cuh"
#include "gemm_naive.cuh"

void launchNaive(const float* dA, const float* dB, float* dC, int M, int N, int K)
{
    const dim3 block(16, 16);
    const dim3 grid((N + block.x - 1) / block.x, (M + block.y - 1) / block.y);
    sgemmNaive<<<grid, block>>>(dA, dB, dC, M, N, K);
}

int main()
{
    return runE6({{"naive", reinterpret_cast<const void*>(sgemmNaive), 256, launchNaive}});
}
