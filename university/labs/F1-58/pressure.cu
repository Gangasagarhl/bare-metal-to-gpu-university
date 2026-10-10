// F1-58 forensic evidence: a kernel that keeps 32 values alive per thread.
// run.sh compiles it twice (default, and with -maxrregcount=16) and keeps ptxas's report.
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

__global__ void polyEval(const float* x, float* y, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) {
        return;
    }
    float acc[32];
#pragma unroll
    for (int k = 0; k < 32; ++k) {
        acc[k] = x[(i + k * 97) % n];
    }
    float s = 0.0f;
#pragma unroll
    for (int r = 0; r < 4; ++r) {
#pragma unroll
        for (int k = 0; k < 32; ++k) {
            s = s * acc[(k + r) % 32] + acc[k];
        }
    }
    y[i] = s;
}

int main()
{
    float* d = nullptr;
    cudaError_t err = cudaMalloc(&d, 1024 * sizeof(float));
    if (err != cudaSuccess) {
        std::fprintf(stderr, "cudaMalloc failed: %s (%s)\n", cudaGetErrorName(err), cudaGetErrorString(err));
        return EXIT_FAILURE;
    }
    polyEval<<<4, 256>>>(d, d, 1024);
    err = cudaDeviceSynchronize();
    std::printf("kernel finished: %s\n", cudaGetErrorString(err));
    cudaFree(d);
    return err == cudaSuccess ? 0 : 1;
}
