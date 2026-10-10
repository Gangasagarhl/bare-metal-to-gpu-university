// F7-15 Listing 2: the library way, with CUB (installed here); hipCUB mirrors this interface.
// Build: nvcc -std=c++17 -O2 -lineinfo -Werror all-warnings cub_device.cu -o cub_device
#include <cub/cub.cuh>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(cudaError_t e, const char* what)
{
    if (e != cudaSuccess) {
        std::printf("%s failed: %s (%s)\n", what, cudaGetErrorName(e), cudaGetErrorString(e));
        std::exit(1);
    }
}

int main()
{
    std::printf("CUB_VERSION %d\n", CUB_VERSION);
    const int n = 1000003;
    std::vector<int> hIn(n);
    for (int i = 0; i < n; ++i) {
        hIn[i] = (i * 7 + 3) % 11;
    }
    int* dIn = nullptr;
    int* dScan = nullptr;
    int* dSum = nullptr;
    check(cudaMalloc(&dIn, n * sizeof(int)), "cudaMalloc(dIn)");
    check(cudaMalloc(&dScan, n * sizeof(int)), "cudaMalloc(dScan)");
    check(cudaMalloc(&dSum, sizeof(int)), "cudaMalloc(dSum)");
    check(cudaMemcpy(dIn, hIn.data(), n * sizeof(int), cudaMemcpyHostToDevice), "cudaMemcpy H2D");

    // Phase 1: ask how much temporary storage each algorithm needs (no work is done).
    void* dTemp = nullptr;
    size_t reduceBytes = 0;
    size_t scanBytes = 0;
    check(cub::DeviceReduce::Sum(dTemp, reduceBytes, dIn, dSum, n), "DeviceReduce::Sum (size query)");
    check(cub::DeviceScan::ExclusiveSum(dTemp, scanBytes, dIn, dScan, n), "DeviceScan::ExclusiveSum (size query)");
    const size_t tempBytes = reduceBytes > scanBytes ? reduceBytes : scanBytes;
    std::printf("temporary storage: reduce %zu bytes, scan %zu bytes\n", reduceBytes, scanBytes);
    check(cudaMalloc(&dTemp, tempBytes), "cudaMalloc(dTemp)");

    // Phase 2: the same calls with real storage do the work, asynchronously on stream 0.
    check(cub::DeviceReduce::Sum(dTemp, reduceBytes, dIn, dSum, n), "DeviceReduce::Sum");
    check(cub::DeviceScan::ExclusiveSum(dTemp, scanBytes, dIn, dScan, n), "DeviceScan::ExclusiveSum");
    check(cudaDeviceSynchronize(), "cudaDeviceSynchronize");

    int hSum = 0;
    std::vector<int> hScan(n);
    check(cudaMemcpy(&hSum, dSum, sizeof(int), cudaMemcpyDeviceToHost), "cudaMemcpy D2H");
    check(cudaMemcpy(hScan.data(), dScan, n * sizeof(int), cudaMemcpyDeviceToHost), "cudaMemcpy D2H");
    long long errors = 0;
    int running = 0;
    for (int i = 0; i < n; ++i) {
        if (hScan[i] != running) {
            ++errors;
        }
        running += hIn[i];
    }
    std::printf("sum = %d (expected %d), scan errors = %lld\n", hSum, running, errors);
    check(cudaFree(dTemp), "cudaFree");
    check(cudaFree(dIn), "cudaFree");
    check(cudaFree(dScan), "cudaFree");
    check(cudaFree(dSum), "cudaFree");
    return (errors == 0 && hSum == running) ? 0 : 1;
}
