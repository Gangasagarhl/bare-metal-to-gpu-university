// vecadd.hip - DR405 F4-42: the HIP program whose path to the GPU this chapter follows.
// Build: hipcc -std=c++17 -O2 --offload-arch=gfx90a vecadd.hip -o vecadd
// The build container has no AMD GPU: the run shows the runtime's own error (AH-26).
#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define CHECK(call)                                                                       \
    do {                                                                                  \
        const hipError_t err_ = (call);                                                   \
        if (err_ != hipSuccess) {                                                         \
            std::fprintf(stderr, "%s:%d: %s failed: %s\n", __FILE__, __LINE__, #call,     \
                         hipGetErrorString(err_));                                        \
            std::exit(1);                                                                 \
        }                                                                                 \
    } while (0)

__global__ void vecAdd(const float* a, const float* b, float* c, int n)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) c[i] = a[i] + b[i];
}

int main()
{
    const int n = 1 << 20;
    std::vector<float> hA(n, 1.0f), hB(n, 2.0f), hC(n, 0.0f);
    int devices = 0;
    CHECK(hipGetDeviceCount(&devices));               // the first call that needs the driver
    float *dA = nullptr, *dB = nullptr, *dC = nullptr;
    const size_t bytes = n * sizeof(float);
    CHECK(hipMalloc(&dA, bytes));
    CHECK(hipMalloc(&dB, bytes));
    CHECK(hipMalloc(&dC, bytes));
    CHECK(hipMemcpy(dA, hA.data(), bytes, hipMemcpyHostToDevice));
    CHECK(hipMemcpy(dB, hB.data(), bytes, hipMemcpyHostToDevice));
    const int block = 256, grid = (n + block - 1) / block;
    vecAdd<<<grid, block>>>(dA, dB, dC, n);           // the launch this chapter follows
    CHECK(hipGetLastError());
    CHECK(hipMemcpy(hC.data(), dC, bytes, hipMemcpyDeviceToHost));
    std::printf("c[0]=%g c[n-1]=%g\n", hC[0], hC[n - 1]);
    CHECK(hipFree(dA));
    CHECK(hipFree(dB));
    CHECK(hipFree(dC));
    return 0;
}
