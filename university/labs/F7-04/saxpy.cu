// saxpy.cu - curriculum E1's SAXPY in CUDA, with a CPU reference check (the port's input).
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s\n", what, cudaGetErrorString(err));
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
    check(cudaMalloc(&dX, bytes), "cudaMalloc dX");
    check(cudaMalloc(&dY, bytes), "cudaMalloc dY");
    check(cudaMemcpy(dX, hX.data(), bytes, cudaMemcpyHostToDevice), "copy x");
    check(cudaMemcpy(dY, hY.data(), bytes, cudaMemcpyHostToDevice), "copy y");

    const int block = 256;
    saxpy<<<(n + block - 1) / block, block>>>(n, a, dX, dY);
    check(cudaGetLastError(), "launch saxpy");
    check(cudaMemcpy(hY.data(), dY, bytes, cudaMemcpyDeviceToHost), "copy back");

    int mismatches = 0;
    for (int i = 0; i < n; ++i) {
        if (hY[i] != ref[i]) {                   // bit-exact: small integers times 3 plus 1
            ++mismatches;
        }
    }
    std::printf("saxpy n=%d: %d mismatches against the CPU reference\n", n, mismatches);
    check(cudaFree(dX), "cudaFree dX");
    check(cudaFree(dY), "cudaFree dY");
    return mismatches == 0 ? 0 : 1;
}
