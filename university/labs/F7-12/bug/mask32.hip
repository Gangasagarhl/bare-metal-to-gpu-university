// F7-12 Listing 4 (bug/mask32.hip): a ballot copied from 32-lane code. Kernel only.
// run.sh compiles it with -Wshorten-64-to-32 -Werror: the compile is expected to fail.
#include <hip/hip_runtime.h>

__global__ void countFlags(const int* flag, int* count)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int mask = __ballot(flag[i] != 0);   // BUG: 64 lanes do not fit in 32 bits
    if (__lane_id() == 0) {
        atomicAdd(count, __popc(mask));
    }
}
