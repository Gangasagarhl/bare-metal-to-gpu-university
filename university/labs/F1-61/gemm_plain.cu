// F1-61 forensic evidence: a "naive" FP32 matrix multiply. run.sh compiles it to SASS.
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

__global__ void gemmNaive(const float* a, const float* b, float* c, int n)
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    if (row < n && col < n) {
        float s = 0.0f;
        for (int k = 0; k < n; ++k) {
            s += a[row * n + k] * b[k * n + col];
        }
        c[row * n + col] = s;
    }
}

int main()
{
    float* p = nullptr;
    cudaError_t err = cudaMalloc(&p, 3 * 64 * 64 * sizeof(float));
    if (err != cudaSuccess) {
        std::fprintf(stderr, "cudaMalloc failed: %s (%s)\n", cudaGetErrorName(err), cudaGetErrorString(err));
        return EXIT_FAILURE;
    }
    gemmNaive<<<dim3(4, 4), dim3(16, 16)>>>(p, p + 4096, p + 8192, 64);
    err = cudaDeviceSynchronize();
    std::printf("kernel finished: %s\n", cudaGetErrorString(err));
    cudaFree(p);
    return err == cudaSuccess ? 0 : 1;
}
