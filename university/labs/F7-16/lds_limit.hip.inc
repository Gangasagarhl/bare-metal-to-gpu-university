// F7-16 lds_limit: one float more LDS than the per-workgroup limit (compile only, expected to fail).
#include <hip/hip_runtime.h>

__global__ void tooBig(float* out)
{
    __shared__ float tile[16384 + 1];         // 65,540 bytes
    tile[threadIdx.x] = static_cast<float>(threadIdx.x);
    __syncthreads();
    out[threadIdx.x] = tile[(threadIdx.x * 7) % (16384 + 1)];
}
