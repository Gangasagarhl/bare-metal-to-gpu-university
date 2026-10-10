// F1-60 Listing 3: the same kernel in HIP, compiled to gfx90a assembly by run.sh.
#include <cstdio>
#include <cstdlib>
#include <hip/hip_runtime.h>

__global__ void fma3(const float* a, const float* b, const float* c, float* d, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        d[i] = a[i] * b[i] + c[i];
    }
}

int main()
{
    float* p = nullptr;
    hipError_t err = hipMalloc(&p, 4 * 1024 * sizeof(float));
    if (err != hipSuccess) {
        std::fprintf(stderr, "hipMalloc failed: %s (%s)\n", hipGetErrorName(err), hipGetErrorString(err));
        return EXIT_FAILURE;
    }
    fma3<<<4, 256>>>(p, p + 1024, p + 2048, p + 3072, 1024);
    err = hipDeviceSynchronize();
    std::printf("kernel finished: %s\n", hipGetErrorString(err));
    (void)hipFree(p);
    return err == hipSuccess ? 0 : 1;
}
