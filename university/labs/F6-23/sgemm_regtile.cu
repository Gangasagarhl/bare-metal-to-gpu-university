// F6-23 Listing 2: E6 steps 1-4 in the harness: naive, tiled, 1-D and 2-D register tiles,
// with and without float4 loads. Untested on hardware. Build with the cuBLAS column:
//   nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 -DUSE_CUBLAS sgemm_regtile.cu -lcublas
#include "../F6-21/gemm_harness.cuh"
#include "../F6-21/gemm_naive.cuh"
#include "../F6-22/gemm_tiled.cuh"
#define LAUNCH(kernel, grid, block, ...) kernel<<<grid, block>>>(__VA_ARGS__)
#include "gemm_regtile.cuh"
#include "launchers.cuh"

void launchNaive(const float* dA, const float* dB, float* dC, int M, int N, int K)
{
    sgemmNaive<<<dim3((N + 15) / 16, (M + 15) / 16), dim3(16, 16)>>>(dA, dB, dC, M, N, K);
}

void launchTiled32(const float* dA, const float* dB, float* dC, int M, int N, int K)
{
    sgemmTiled<32><<<dim3((N + 31) / 32, (M + 31) / 32), dim3(32, 32)>>>(dA, dB, dC, M, N, K);
}

template <typename T>
const void* fn(T kernel)
{
    return reinterpret_cast<const void*>(kernel);
}

int main()
{
    return runE6({
        {"naive", fn(sgemmNaive), 256, launchNaive},
        {"tiled32", fn(sgemmTiled<32>), 1024, launchTiled32},
        {"reg1d 64x64/8", fn(sgemmRegTile<64, 64, 8, 8, 1, false>), 512,
         launchRegTile<64, 64, 8, 8, 1, false>},
        {"reg2d 64x64/4x4", fn(sgemmRegTile<64, 64, 8, 4, 4, false>), 256,
         launchRegTile<64, 64, 8, 4, 4, false>},
        {"vec 64x64/4x4", fn(sgemmRegTile<64, 64, 8, 4, 4, true>), 256,
         launchRegTile<64, 64, 8, 4, 4, true>},
        {"vec 128x128/8x8", fn(sgemmRegTile<128, 128, 8, 8, 8, true>), 256,
         launchRegTile<128, 128, 8, 8, 8, true>},
    });
}
