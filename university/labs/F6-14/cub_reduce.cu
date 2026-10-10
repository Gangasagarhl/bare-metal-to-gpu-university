// F6-14 Listing 2: the reference the E3 target is measured against: cub::DeviceReduce::Sum
// (CUB 2.0.1 as installed). Two calls: the first asks how much temporary storage is needed,
// the second does the work. Built for real; untested on hardware.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cub/cub.cuh>
#include <cuda_runtime.h>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    int dev = 0;
    check(cudaGetDevice(&dev), "cudaGetDevice");
    const int n = 1 << 28;              // CUB 2.0.1 takes num_items as int (see the header)
    std::vector<float> h(n);
    for (int i = 0; i < n; ++i) {
        h[i] = static_cast<float>((static_cast<unsigned>(i) * 2654435761u) % 1000) / 1000.0f;
    }
    float* dIn = nullptr;
    float* dOut = nullptr;
    check(cudaMalloc(&dIn, sizeof(float) * n), "malloc in");
    check(cudaMalloc(&dOut, sizeof(float)), "malloc out");
    check(cudaMemcpy(dIn, h.data(), sizeof(float) * n, cudaMemcpyHostToDevice), "copy in");
    void* dTemp = nullptr;
    size_t tempBytes = 0;
    check(cub::DeviceReduce::Sum(dTemp, tempBytes, dIn, dOut, n), "Sum (size query)");
    check(cudaMalloc(&dTemp, tempBytes), "malloc temp");
    cudaEvent_t start, stop;
    check(cudaEventCreate(&start), "event");
    check(cudaEventCreate(&stop), "event");
    std::vector<float> ms(20);
    check(cub::DeviceReduce::Sum(dTemp, tempBytes, dIn, dOut, n), "Sum (warm-up)");
    for (float& m : ms) {
        check(cudaEventRecord(start), "record");
        check(cub::DeviceReduce::Sum(dTemp, tempBytes, dIn, dOut, n), "Sum");
        check(cudaEventRecord(stop), "record");
        check(cudaEventSynchronize(stop), "sync");
        check(cudaEventElapsedTime(&m, start, stop), "elapsed");
    }
    std::sort(ms.begin(), ms.end());
    float result = 0.0f;
    check(cudaMemcpy(&result, dOut, sizeof result, cudaMemcpyDeviceToHost), "copy out");
    const float med = (ms[9] + ms[10]) / 2;
    std::printf("cub::DeviceReduce::Sum: %.3f, temp storage %zu bytes, median %.3f ms, %.1f GB/s\n",
                result, tempBytes, med, static_cast<double>(n) * sizeof(float) / (med * 1.0e6));
    check(cudaFree(dTemp), "free temp");
    check(cudaFree(dIn), "free in");
    check(cudaFree(dOut), "free out");
    return 0;
}
