// F1-60 Listing 2: three independent loads, then one fused multiply-add per thread.
// run.sh compiles it to SASS; the run itself records the no-GPU error (untested on hardware).
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

__global__ void fma3(const float* a, const float* b, const float* c, float* d, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        d[i] = a[i] * b[i] + c[i];     // the three loads do not depend on each other
    }
}

int main()
{
    float* p = nullptr;
    cudaError_t err = cudaMalloc(&p, 4 * 1024 * sizeof(float));
    if (err != cudaSuccess) {
        std::fprintf(stderr, "cudaMalloc failed: %s (%s)\n", cudaGetErrorName(err), cudaGetErrorString(err));
        return EXIT_FAILURE;
    }
    fma3<<<4, 256>>>(p, p + 1024, p + 2048, p + 3072, 1024);
    err = cudaDeviceSynchronize();
    std::printf("kernel finished: %s\n", cudaGetErrorString(err));
    cudaFree(p);
    return err == cudaSuccess ? 0 : 1;
}
