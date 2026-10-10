// devquery.hip - print the facts about each GPU that a compatibility check needs.
#include <hip/hip_runtime.h>
#include <cstdio>

static bool check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::printf("%s failed: %s (%s)\n", what, hipGetErrorName(err), hipGetErrorString(err));
        return false;
    }
    return true;
}

__global__ void readWarpSize(int* out)
{
    // the compiler's value of warpSize for the code object that really runs
    if (threadIdx.x == 0) {
        out[0] = warpSize;
    }
}

int main()
{
    int runtime = 0;
    int driver = 0;
    if (!check(hipRuntimeGetVersion(&runtime), "hipRuntimeGetVersion")) {
        return 1;
    }
    std::printf("HIP runtime version number: %d\n", runtime);
    if (check(hipDriverGetVersion(&driver), "hipDriverGetVersion")) {
        std::printf("driver version number: %d\n", driver);
    }

    int count = 0;
    if (!check(hipGetDeviceCount(&count), "hipGetDeviceCount")) {
        return 1;
    }
    for (int d = 0; d < count; ++d) {
        hipDeviceProp_t p{};
        if (!check(hipGetDeviceProperties(&p, d), "hipGetDeviceProperties")) {
            return 1;
        }
        std::printf("device %d: %s\n", d, p.name);
        std::printf("  gcnArchName (GPU target): %s\n", p.gcnArchName);
        std::printf("  warpSize: %d\n", p.warpSize);
        std::printf("  multiProcessorCount: %d\n", p.multiProcessorCount);
        std::printf("  totalGlobalMem: %zu bytes\n", p.totalGlobalMem);
        std::printf("  sharedMemPerBlock: %zu bytes\n", p.sharedMemPerBlock);
        std::printf("  maxThreadsPerBlock: %d\n", p.maxThreadsPerBlock);
        std::printf("  compute capability fields: %d.%d\n", p.major, p.minor);

        int* dWs = nullptr;
        int hWs = 0;
        if (!check(hipSetDevice(d), "hipSetDevice") ||
            !check(hipMalloc(&dWs, sizeof(int)), "hipMalloc")) {
            return 1;
        }
        readWarpSize<<<1, 1>>>(dWs);
        const bool ok = check(hipGetLastError(), "launch") &&
                        check(hipMemcpy(&hWs, dWs, sizeof(int), hipMemcpyDeviceToHost),
                              "hipMemcpy");
        (void)hipFree(dWs);
        if (!ok) {
            return 1;
        }
        std::printf("  warpSize seen inside a kernel: %d\n", hWs);
    }
    return 0;
}
