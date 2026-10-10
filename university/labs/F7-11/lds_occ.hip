// F7-11 Listing 3: the same small kernel with 4, 8, 16, 32 and 64 KiB of LDS per workgroup,
// to read how the compiler's occupancy figure falls as the LDS per workgroup grows.
// Complete program; the build container has no AMD GPU, so the run stops at the first call.
#include <cstdio>
#include <cstdlib>
#include <hip/hip_runtime.h>

template <int BYTES>
__global__ void __launch_bounds__(256) ldsUse(const float* x, float* y)
{
    __shared__ float buf[BYTES / 4];
    int t = threadIdx.x;
    for (int k = t; k < BYTES / 4; k += 256) {
        buf[k] = x[blockIdx.x * (BYTES / 4) + k];
    }
    __syncthreads();
    y[blockIdx.x * 256 + t] = buf[(t * 7) % (BYTES / 4)];
}

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err), hipGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

template <int BYTES>
void launch(const float* dX, float* dY)
{
    ldsUse<BYTES><<<16, 256>>>(dX, dY);
    check(hipGetLastError(), "launch ldsUse");
}

int main()
{
    float *dX = nullptr, *dY = nullptr;
    check(hipMalloc(&dX, 16 * 65536), "hipMalloc x");
    check(hipMalloc(&dY, 16 * 256 * sizeof(float)), "hipMalloc y");
    launch<4096>(dX, dY);
    launch<8192>(dX, dY);
    launch<16384>(dX, dY);
    launch<32768>(dX, dY);
    launch<65536>(dX, dY);
    check(hipDeviceSynchronize(), "hipDeviceSynchronize");
    check(hipFree(dX), "hipFree x");
    check(hipFree(dY), "hipFree y");
    return 0;
}
