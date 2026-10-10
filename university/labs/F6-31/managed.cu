// F6-31 Listing 1: managed memory with and without prefetching.
// One pointer is used by the CPU and the GPU. Run 1 lets pages migrate on demand;
// run 2 prefetches to the GPU before the kernel and back to the CPU after it.
// Prefetching to a GPU needs cudaDevAttrConcurrentManagedAccess (checked first).
#include <cstdio>
#include "../F6-05/cuda_check.h"

__global__ void scaleAdd(float* x, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        x[i] = 2.0f * x[i] + 1.0f;
    }
}

// Fills x on the CPU, runs the kernel, reads x on the CPU. Returns kernel+wait ms.
float runOnce(float* x, int n, int device, bool prefetch, cudaStream_t stream, long long* errors)
{
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    for (int i = 0; i < n; ++i) {
        x[i] = static_cast<float>(i % 100);               // CPU touches every page first
    }
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));
    CUDA_CHECK(cudaEventRecord(start, stream));
    if (prefetch) {
        CUDA_CHECK(cudaMemPrefetchAsync(x, bytes, device, stream));       // to the GPU
    }
    scaleAdd<<<blocks, threads, 0, stream>>>(x, n);
    CUDA_CHECK_LAUNCH();
    if (prefetch) {
        CUDA_CHECK(cudaMemPrefetchAsync(x, bytes, cudaCpuDeviceId, stream));  // back to the CPU
    }
    CUDA_CHECK(cudaEventRecord(stop, stream));
    CUDA_CHECK(cudaEventSynchronize(stop));               // the CPU must not touch x before this
    float ms = 0.0f;
    CUDA_CHECK(cudaEventElapsedTime(&ms, start, stop));
    *errors = 0;
    for (int i = 0; i < n; ++i) {
        if (x[i] != 2.0f * static_cast<float>(i % 100) + 1.0f) {
            ++*errors;
        }
    }
    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    return ms;
}

int main()
{
    const int n = 1 << 24;
    const std::size_t bytes = static_cast<std::size_t>(n) * sizeof(float);
    int device = 0;
    CUDA_CHECK(cudaGetDevice(&device));
    int managed = 0;
    int concurrent = 0;
    CUDA_CHECK(cudaDeviceGetAttribute(&managed, cudaDevAttrManagedMemory, device));
    CUDA_CHECK(cudaDeviceGetAttribute(&concurrent, cudaDevAttrConcurrentManagedAccess, device));
    std::printf("managedMemory %d, concurrentManagedAccess %d\n", managed, concurrent);
    if (managed == 0) {
        std::printf("this device cannot allocate managed memory\n");
        return 1;
    }
    float* x = nullptr;
    CUDA_CHECK(cudaMallocManaged(&x, bytes));             // one pointer, valid on CPU and GPU
    cudaStream_t stream = nullptr;
    CUDA_CHECK(cudaStreamCreate(&stream));

    long long errors = 0;
    (void)runOnce(x, n, device, false, stream, &errors);  // warm-up
    const float demandMs = runOnce(x, n, device, false, stream, &errors);
    std::printf("on-demand migration: %.3f ms, %lld errors\n", demandMs, errors);
    if (concurrent != 0) {
        const float prefetchMs = runOnce(x, n, device, true, stream, &errors);
        std::printf("with prefetch:       %.3f ms, %lld errors\n", prefetchMs, errors);
    } else {
        std::printf("prefetch to the GPU skipped: needs concurrentManagedAccess = 1\n");
    }

    CUDA_CHECK(cudaStreamDestroy(stream));
    CUDA_CHECK(cudaFree(x));                              // managed memory: cudaFree
    return errors == 0 ? 0 : 1;
}
