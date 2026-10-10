// F1-63 Listing 2: a probe kernel whose compiled form reveals a fact about the target:
// the compiler replaces warpSize by the target's wavefront size. The run records the no-GPU error.
#include <cstdio>
#include <hip/hip_runtime.h>

__global__ void probe(int* out)
{
    out[threadIdx.x] = warpSize;
}

int main()
{
    int* d = nullptr;
    hipError_t err = hipMalloc(&d, 64 * sizeof(int));
    std::printf("hipMalloc: %s\n", hipGetErrorString(err));
    if (err != hipSuccess) {
        return 1;
    }
    probe<<<1, 64>>>(d);
    err = hipDeviceSynchronize();
    std::printf("probe: %s\n", hipGetErrorString(err));
    (void)hipFree(d);
    return err == hipSuccess ? 0 : 1;
}
