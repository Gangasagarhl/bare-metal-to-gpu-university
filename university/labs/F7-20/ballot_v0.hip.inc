// F7-20 nv_ballot_v0: the first try, plain __ballot, built through HIP's NVIDIA path for sm_80.
#include <hip/hip_runtime.h>

__global__ void countPositive(const float* x, int n, unsigned long long* count)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    const bool positive = (i < n) && (x[i] > 0.0f);
    const unsigned long long mask = __ballot(positive);
    if (threadIdx.x % warpSize == 0) {
        atomicAdd(count, static_cast<unsigned long long>(__popcll(mask)));
    }
}
