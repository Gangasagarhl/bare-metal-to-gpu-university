// saxpy.cu - curriculum E1's SAXPY in CUDA, with a CPU reference check (the port's input).
#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s\n", what, hipGetErrorString(err));
        std::exit(1);
    }
}

__global__ void saxpy(int n, float a, const float* x, float* y)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = a * x[i] + y[i];
    }
}

int main()
{
    const int n = 1000003;                       // not a multiple of the block size
    const float a = 3.0f;
    std::vector<float> hX(n);
    std::vector<float> hY(n);
    for (int i = 0; i < n; ++i) {
        hX[i] = static_cast<float>(i % 1024);
        hY[i] = 1.0f;
    }
    std::vector<float> ref(n);
    for (int i = 0; i < n; ++i) {
        ref[i] = a * hX[i] + hY[i];              // CPU reference
    }

    float* dX = nullptr;
    float* dY = nullptr;
    const size_t bytes = n * sizeof(float);
    check(hipMalloc(&dX, bytes), "hipMalloc dX");
    check(hipMalloc(&dY, bytes), "hipMalloc dY");
    check(hipMemcpy(dX, hX.data(), bytes, hipMemcpyHostToDevice), "copy x");
    check(hipMemcpy(dY, hY.data(), bytes, hipMemcpyHostToDevice), "copy y");

    const int block = 256;
    saxpy<<<(n + block - 1) / block, block>>>(n, a, dX, dY);
    check(hipGetLastError(), "launch saxpy");
    check(hipMemcpy(hY.data(), dY, bytes, hipMemcpyDeviceToHost), "copy back");

    int mismatches = 0;
    for (int i = 0; i < n; ++i) {
        if (hY[i] != ref[i]) {                   // bit-exact: small integers times 3 plus 1
            ++mismatches;
        }
    }
    std::printf("saxpy n=%d: %d mismatches against the CPU reference\n", n, mismatches);
    check(hipFree(dX), "hipFree dX");
    check(hipFree(dY), "hipFree dY");
    return mismatches == 0 ? 0 : 1;
}
