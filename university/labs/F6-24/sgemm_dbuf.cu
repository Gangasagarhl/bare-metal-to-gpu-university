// F6-24 Listing 2: E6 step 5 in the harness: double buffering with asynchronous copies,
// next to the best F6-23 kernels. Untested on hardware. Build (as run.sh does):
//   nvcc -std=c++17 -O2 -lineinfo -arch=sm_80 -DUSE_CUBLAS sgemm_dbuf.cu -lcublas
// (cp.async needs sm_80 or newer; for older targets the header falls back to plain copies.)
#include <cuda_pipeline.h>
#include "../F6-21/gemm_harness.cuh"
#define LAUNCH(kernel, grid, block, ...) kernel<<<grid, block>>>(__VA_ARGS__)
#include "../F6-23/gemm_regtile.cuh"
#include "../F6-23/launchers.cuh"
#include "gemm_dbuf.cuh"

template <typename T>
const void* fn(T kernel)
{
    return reinterpret_cast<const void*>(kernel);
}

int main()
{
    return runE6({
        {"vec 64x64/4x4", fn(sgemmRegTile<64, 64, 8, 4, 4, true>), 256,
         launchRegTile<64, 64, 8, 4, 4, true>},
        {"vec 128x128/8x8", fn(sgemmRegTile<128, 128, 8, 8, 8, true>), 256,
         launchRegTile<128, 128, 8, 8, 8, true>},
        {"dbuf 64x64/4x4", fn(sgemmDoubleBuffer<64, 64, 8, 4, 4>), 256,
         launchDoubleBuffer<64, 64, 8, 4, 4>},
        {"dbuf 128x128/8x8", fn(sgemmDoubleBuffer<128, 128, 8, 8, 8>), 256,
         launchDoubleBuffer<128, 128, 8, 8, 8>},
    });
}
