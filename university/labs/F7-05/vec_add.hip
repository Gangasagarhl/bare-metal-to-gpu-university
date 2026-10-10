// vec_add.hip - vector add in HIP: C = A + B, checked against the CPU.
#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err),
                     hipGetErrorString(err));
        std::exit(1);
    }
}

__global__ void vecAdd(const float* a, const float* b, float* c, int n)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}

int main()
{
    const int n = 1000003;
    const size_t bytes = n * sizeof(float);
    std::vector<float> hA(n);
    std::vector<float> hB(n);
    std::vector<float> hC(n, -1.0f);
    for (int i = 0; i < n; ++i) {
        hA[i] = static_cast<float>(i);
        hB[i] = static_cast<float>(2 * i);
    }

    float* dA = nullptr;
    float* dB = nullptr;
    float* dC = nullptr;
    check(hipMalloc(&dA, bytes), "hipMalloc dA");
    check(hipMalloc(&dB, bytes), "hipMalloc dB");
    check(hipMalloc(&dC, bytes), "hipMalloc dC");
    check(hipMemcpy(dA, hA.data(), bytes, hipMemcpyHostToDevice), "copy A");
    check(hipMemcpy(dB, hB.data(), bytes, hipMemcpyHostToDevice), "copy B");

    const int block = 256;
    const int grid = (n + block - 1) / block;
    vecAdd<<<grid, block>>>(dA, dB, dC, n);
    check(hipGetLastError(), "launch vecAdd");
    check(hipDeviceSynchronize(), "kernel vecAdd");
    check(hipMemcpy(hC.data(), dC, bytes, hipMemcpyDeviceToHost), "copy C");

    int errors = 0;
    for (int i = 0; i < n; ++i) {
        if (hC[i] != hA[i] + hB[i]) {
            ++errors;
        }
    }
    std::printf("vecAdd n=%d grid=%d block=%d: %d errors\n", n, grid, block, errors);
    check(hipFree(dA), "hipFree dA");
    check(hipFree(dB), "hipFree dB");
    check(hipFree(dC), "hipFree dC");
    return errors == 0 ? 0 : 1;
}
