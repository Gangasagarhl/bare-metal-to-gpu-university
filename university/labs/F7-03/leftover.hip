// leftover.hip - a half-finished port: the kernel and most calls are HIP,
// but the stream is still declared and created with its CUDA names.
#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdlib>

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s\n", what, hipGetErrorString(err));
        std::exit(1);
    }
}

__global__ void fill(int* x, int value, int n)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        x[i] = value;
    }
}

int main()
{
    const int n = 4096;
    int* dX = nullptr;
    check(hipMalloc(&dX, n * sizeof(int)), "hipMalloc");
    cudaStream_t stream;                                    // left over from the CUDA version
    cudaStreamCreate(&stream);                              // left over, and unchecked
    fill<<<(n + 255) / 256, 256, 0, stream>>>(dX, 7, n);
    check(hipGetLastError(), "launch fill");
    check(hipStreamSynchronize(stream), "hipStreamSynchronize");
    std::printf("filled %d elements\n", n);
    check(hipFree(dX), "hipFree");
    return 0;
}
