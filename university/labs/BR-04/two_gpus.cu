// BR-04 Listing 8: what carries over and what changes, in CUDA. The tools of one GPU
// (a kernel, a stream, events, error checks) are used unchanged, once per device; what is
// new is the current device, the peer-access question for each ordered pair, and a copy
// between two device memories over whatever link connects them.
// Build: nvcc -std=c++17 -O2 -lineinfo -Werror all-warnings two_gpus.cu -o two_gpus
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define CHECK(call)                                                                       \
    do {                                                                                  \
        cudaError_t err_ = (call);                                                        \
        if (err_ != cudaSuccess) {                                                        \
            std::printf("%s failed: %s (%s)\n", #call, cudaGetErrorName(err_),            \
                        cudaGetErrorString(err_));                                        \
            std::exit(1);                                                                 \
        }                                                                                 \
    } while (0)

__global__ void scale(float* g, int n, float s)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        g[i] *= s;
    }
}

int main()
{
    int count = 0;
    CHECK(cudaGetDeviceCount(&count));
    if (count < 2) {
        std::printf("this listing needs two GPUs; found %d\n", count);
        return 1;
    }
    const int n = 1 << 24;                                       // 16 Mi floats = 64 MiB per GPU
    std::vector<float*> grad(2);
    std::vector<cudaStream_t> stream(2);
    for (int d = 0; d < 2; ++d) {                                // one GPU's recipe, once per device
        CHECK(cudaSetDevice(d));
        CHECK(cudaMalloc(&grad[d], n * sizeof(float)));
        CHECK(cudaStreamCreate(&stream[d]));
        CHECK(cudaMemsetAsync(grad[d], 0, n * sizeof(float), stream[d]));
        scale<<<(n + 255) / 256, 256, 0, stream[d]>>>(grad[d], n, 0.5f);
        CHECK(cudaGetLastError());
    }
    int can01 = 0;
    int can10 = 0;
    CHECK(cudaDeviceCanAccessPeer(&can01, 0, 1));                // an ordered pair: ask both ways
    CHECK(cudaDeviceCanAccessPeer(&can10, 1, 0));
    std::printf("peer access 0->1: %d, 1->0: %d\n", can01, can10);
    if (can01) {
        CHECK(cudaSetDevice(0));
        CHECK(cudaDeviceEnablePeerAccess(1, 0));
    }
    cudaEvent_t start;
    cudaEvent_t stop;
    CHECK(cudaSetDevice(1));
    CHECK(cudaEventCreate(&start));
    CHECK(cudaEventCreate(&stop));
    CHECK(cudaStreamSynchronize(stream[1]));
    CHECK(cudaSetDevice(0));
    CHECK(cudaStreamSynchronize(stream[0]));
    CHECK(cudaSetDevice(1));
    CHECK(cudaEventRecord(start, stream[1]));                    // GPU 1 -> GPU 0, on GPU 1's stream
    CHECK(cudaMemcpyPeerAsync(grad[0], 0, grad[1], 1, n * sizeof(float), stream[1]));
    CHECK(cudaEventRecord(stop, stream[1]));
    CHECK(cudaEventSynchronize(stop));
    float ms = 0;
    CHECK(cudaEventElapsedTime(&ms, start, stop));
    std::printf("one copy of %zu bytes GPU 1 -> GPU 0: %.3f ms (one run: no warm-up, no median)\n",
                n * sizeof(float), ms);
    for (int d = 0; d < 2; ++d) {
        CHECK(cudaSetDevice(d));
        CHECK(cudaStreamDestroy(stream[d]));
        CHECK(cudaFree(grad[d]));
    }
    CHECK(cudaSetDevice(1));
    CHECK(cudaEventDestroy(start));
    CHECK(cudaEventDestroy(stop));
    return 0;
}
