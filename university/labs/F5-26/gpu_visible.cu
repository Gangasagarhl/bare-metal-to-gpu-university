// gpu_visible.cu - what a job sees of the GPUs a scheduler gave it: the value of
// CUDA_VISIBLE_DEVICES (which the CUDA 12.0 headers recommend for restricting the GPUs
// CUDA uses) and the device count the runtime reports (F5-26, DS303).
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

int main()
{
    const char* visible = std::getenv("CUDA_VISIBLE_DEVICES");
    std::printf("CUDA_VISIBLE_DEVICES = %s\n", visible != nullptr ? visible : "(not set)");

    int count = 0;
    const cudaError_t err = cudaGetDeviceCount(&count);
    if (err != cudaSuccess) {
        std::printf("cudaGetDeviceCount failed: %s (%s)\n", cudaGetErrorName(err),
                    cudaGetErrorString(err));
        return 1;
    }
    std::printf("devices visible to this process: %d\n", count);
    for (int d = 0; d < count; ++d) {
        cudaDeviceProp prop{};
        if (cudaGetDeviceProperties(&prop, d) == cudaSuccess) {
            std::printf("  device %d: %s, PCI bus %d\n", d, prop.name, prop.pciBusID);
        }
    }
    return 0;
}
