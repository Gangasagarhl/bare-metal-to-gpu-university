// warp_size.hip - what warpSize is inside a kernel, for the target the code was built for.
#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdlib>

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err),
                     hipGetErrorString(err));
        std::exit(1);
    }
}

__global__ void writeWarpSize(int* out)
{
    out[0] = warpSize;
}

int main()
{
    int* dOut = nullptr;
    check(hipMalloc(&dOut, sizeof(int)), "hipMalloc");
    writeWarpSize<<<1, 1>>>(dOut);
    check(hipGetLastError(), "launch writeWarpSize");
    int hOut = 0;
    check(hipMemcpy(&hOut, dOut, sizeof(int), hipMemcpyDeviceToHost), "hipMemcpy");
    std::printf("warpSize inside the kernel: %d\n", hOut);
    check(hipFree(dOut), "hipFree");
    return 0;
}
