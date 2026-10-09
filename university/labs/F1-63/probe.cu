// F1-63 Listing 3: a probe kernel for NVIDIA targets. PTX keeps the symbol WARP_SZ;
// the SASS for a real target shows the value. The run records the no-GPU error.
#include <cstdio>
#include <cuda_runtime.h>

__global__ void probe(int* out)
{
    out[threadIdx.x] = warpSize;
}

int main()
{
    int* d = nullptr;
    cudaError_t err = cudaMalloc(&d, 64 * sizeof(int));
    std::printf("cudaMalloc: %s\n", cudaGetErrorString(err));
    if (err != cudaSuccess) {
        return 1;
    }
    probe<<<1, 64>>>(d);
    err = cudaDeviceSynchronize();
    std::printf("probe: %s\n", cudaGetErrorString(err));
    cudaFree(d);
    return err == cudaSuccess ? 0 : 1;
}
