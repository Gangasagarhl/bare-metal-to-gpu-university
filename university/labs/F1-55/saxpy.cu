// F1-55 Listing 3: a throughput-shaped job (SAXPY) written for a GPU.
// Every element is independent: there is no chain. Built for real; the build
// container has no GPU, so the run records the runtime's own error (AH-26).
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

__global__ void saxpy(int n, float a, const float* x, float* y)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = a * x[i] + y[i];
    }
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int n = 1 << 20;
    const std::size_t bytes = n * sizeof(float);
    std::vector<float> hX(n, 1.0f), hY(n, 2.0f);

    int devices = 0;
    check(cudaGetDeviceCount(&devices), "cudaGetDeviceCount");

    float* dX = nullptr;
    float* dY = nullptr;
    check(cudaMalloc(&dX, bytes), "cudaMalloc x");
    check(cudaMalloc(&dY, bytes), "cudaMalloc y");
    check(cudaMemcpy(dX, hX.data(), bytes, cudaMemcpyHostToDevice), "copy x");
    check(cudaMemcpy(dY, hY.data(), bytes, cudaMemcpyHostToDevice), "copy y");
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;
    saxpy<<<blocks, threads>>>(n, 3.0f, dX, dY);
    check(cudaGetLastError(), "kernel launch");
    check(cudaMemcpy(hY.data(), dY, bytes, cudaMemcpyDeviceToHost), "copy y back");
    int errors = 0;
    for (int i = 0; i < n; ++i) {
        if (hY[i] != 5.0f) { ++errors; }
    }
    std::printf("checked %d elements, %d errors\n", n, errors);
    check(cudaFree(dX), "cudaFree x");
    check(cudaFree(dY), "cudaFree y");
    return errors == 0 ? 0 : 1;
}
