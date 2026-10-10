// F7-08 Listing 3 (wave_info_cuda.cu): the same kernel in CUDA, compiled only to read how warpSize is encoded.
#include <cstdio>

__global__ void waveInfo(int* out)
{
    int t = threadIdx.x;
    out[3 * t + 0] = warpSize;
    out[3 * t + 1] = t / warpSize;
    out[3 * t + 2] = t % warpSize;
}

int main()
{
    int* dOut = nullptr;
    cudaError_t err = cudaMalloc(&dOut, 3 * 256 * sizeof(int));
    if (err != cudaSuccess) {
        std::fprintf(stderr, "cudaMalloc failed: %s\n", cudaGetErrorString(err));
        return 1;
    }
    waveInfo<<<1, 256>>>(dOut);
    err = cudaDeviceSynchronize();
    std::printf("kernel finished: %s\n", cudaGetErrorString(err));
    cudaFree(dOut);
    return err == cudaSuccess ? 0 : 1;
}
