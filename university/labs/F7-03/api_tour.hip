// api_tour.hip - the CUDA runtime calls of CU201, written with their HIP names.
// scale x by a in a stream, time it with events, copy back, check every element.
#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdlib>

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err),
                     hipGetErrorString(err));
        std::exit(1);
    }
}

__global__ void scale(float* x, float a, int n)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        x[i] = a * x[i];
    }
}

int main()
{
    const int n = 1 << 20;
    const size_t bytes = n * sizeof(float);

    float* hX = nullptr;                                   // pinned host buffer
    check(hipHostMalloc(reinterpret_cast<void**>(&hX), bytes, hipHostMallocDefault),
          "hipHostMalloc");
    for (int i = 0; i < n; ++i) {
        hX[i] = static_cast<float>(i % 1000);
    }

    float* dX = nullptr;
    check(hipMalloc(&dX, bytes), "hipMalloc");
    hipStream_t stream;
    check(hipStreamCreate(&stream), "hipStreamCreate");
    hipEvent_t start;
    hipEvent_t stop;
    check(hipEventCreate(&start), "hipEventCreate start");
    check(hipEventCreate(&stop), "hipEventCreate stop");

    const int block = 256;
    const int grid = (n + block - 1) / block;
    check(hipEventRecord(start, stream), "hipEventRecord start");
    check(hipMemcpyAsync(dX, hX, bytes, hipMemcpyHostToDevice, stream), "copy in");
    scale<<<grid, block, 0, stream>>>(dX, 2.0f, n);
    check(hipGetLastError(), "launch scale");
    check(hipMemcpyAsync(hX, dX, bytes, hipMemcpyDeviceToHost, stream), "copy out");
    check(hipEventRecord(stop, stream), "hipEventRecord stop");
    check(hipEventSynchronize(stop), "hipEventSynchronize");

    float ms = 0.0f;
    check(hipEventElapsedTime(&ms, start, stop), "hipEventElapsedTime");
    int errors = 0;
    for (int i = 0; i < n; ++i) {
        if (hX[i] != 2.0f * static_cast<float>(i % 1000)) {
            ++errors;
        }
    }
    std::printf("checked %d elements, %d errors; copy+kernel+copy took %.3f ms\n", n, errors, ms);

    check(hipEventDestroy(start), "hipEventDestroy start");
    check(hipEventDestroy(stop), "hipEventDestroy stop");
    check(hipStreamDestroy(stream), "hipStreamDestroy");
    check(hipFree(dX), "hipFree");
    check(hipHostFree(hX), "hipHostFree");
    return errors == 0 ? 0 : 1;
}
