// hello_hip.hip - which HIP platform was this program built for, and does it see a GPU?
#include <hip/hip_runtime.h>
#include <cstdio>

__global__ void writeLaneCount(int* out)
{
    // warpSize is a device-side constant: 32 or 64 depending on the target (F7-07)
    if (blockIdx.x == 0 && threadIdx.x == 0) {
        out[0] = warpSize;
    }
}

int main()
{
#if defined(__HIP_PLATFORM_AMD__)
    std::printf("built for platform: AMD (ROCm back end)\n");
#elif defined(__HIP_PLATFORM_NVIDIA__)
    std::printf("built for platform: NVIDIA (CUDA back end)\n");
#else
    std::printf("built for platform: unknown\n");
#endif
    std::printf("HIP headers: version %d.%d\n", HIP_VERSION_MAJOR, HIP_VERSION_MINOR);

    int count = 0;
    const hipError_t err = hipGetDeviceCount(&count);
    if (err != hipSuccess) {
        std::printf("hipGetDeviceCount failed: %s (%s)\n", hipGetErrorName(err),
                    hipGetErrorString(err));
        return 1;
    }
    std::printf("devices: %d\n", count);

    int* dOut = nullptr;
    if (hipMalloc(&dOut, sizeof(int)) != hipSuccess) {
        std::printf("hipMalloc failed\n");
        return 1;
    }
    writeLaneCount<<<1, 1>>>(dOut);
    const hipError_t launch = hipGetLastError();
    if (launch != hipSuccess) {
        std::printf("launch failed: %s\n", hipGetErrorString(launch));
        (void)hipFree(dOut);
        return 1;
    }
    int hOut = 0;
    const hipError_t copy = hipMemcpy(&hOut, dOut, sizeof(int), hipMemcpyDeviceToHost);
    (void)hipFree(dOut);
    if (copy != hipSuccess) {
        std::printf("kernel or copy failed: %s\n", hipGetErrorString(copy));
        return 1;
    }
    std::printf("lanes per warp/wavefront on device 0: %d\n", hOut);
    return 0;
}
