// F7-10 Listing 2: register pressure on purpose. Each work-item keeps K running values
// in registers. accum<K> uses the default launch bounds; accumLB<K> promises at most
// 256 work-items per workgroup with __launch_bounds__(256).
// Complete program; the build container has no AMD GPU, so the run stops at the first call.
#include <cstdio>
#include <cstdlib>
#include <hip/hip_runtime.h>

template <int K>
__device__ float work(const float* x, int i, int n, int iters)
{
    float a[K];
#pragma unroll
    for (int k = 0; k < K; ++k) {
        a[k] = x[(i + k * 97) % n];
    }
    for (int t = 0; t < iters; ++t) {
#pragma unroll
        for (int k = 0; k < K; ++k) {
            a[k] = a[k] * a[(k + 1) % K] + 1.0f;   // every a[k] stays live across the loop
        }
    }
    float s = 0.0f;
#pragma unroll
    for (int k = 0; k < K; ++k) {
        s += a[k];
    }
    return s;
}

template <int K>
__global__ void accum(const float* x, float* y, int n, int iters)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = work<K>(x, i, n, iters);
    }
}

template <int K>
__global__ void __launch_bounds__(256) accumLB(const float* x, float* y, int n, int iters)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = work<K>(x, i, n, iters);
    }
}

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err), hipGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

template <int K>
void launchBoth(const float* dX, float* dY, int n)
{
    accum<K><<<n / 256, 256>>>(dX, dY, n, 100);
    check(hipGetLastError(), "launch accum");
    accumLB<K><<<n / 256, 256>>>(dX, dY, n, 100);
    check(hipGetLastError(), "launch accumLB");
}

int main()
{
    const int n = 1 << 16;
    float* dX = nullptr;
    float* dY = nullptr;
    check(hipMalloc(&dX, n * sizeof(float)), "hipMalloc x");
    check(hipMalloc(&dY, n * sizeof(float)), "hipMalloc y");
    check(hipMemset(dX, 0, n * sizeof(float)), "hipMemset x");
    launchBoth<8>(dX, dY, n);
    launchBoth<32>(dX, dY, n);
    launchBoth<64>(dX, dY, n);
    launchBoth<96>(dX, dY, n);
    launchBoth<128>(dX, dY, n);
    launchBoth<200>(dX, dY, n);
    launchBoth<256>(dX, dY, n);
    check(hipDeviceSynchronize(), "hipDeviceSynchronize");
    std::printf("all launches finished\n");
    check(hipFree(dX), "hipFree x");
    check(hipFree(dY), "hipFree y");
    return 0;
}
