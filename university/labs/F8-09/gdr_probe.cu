// F8-09 Listing 3: ask the CUDA runtime, per GPU, whether the device supports the
// GPUDirect RDMA APIs (cudaDevAttrGPUDirectRDMASupported, driver_types.h of CUDA 12.0).
// "Supported by the GPU" is necessary, not sufficient: the NIC, its driver, the kernel
// module that connects them and the PCIe topology must all allow it too (see the chapter).
#include <cuda_runtime.h>

#include <cstdio>

static bool check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::printf("CUDA error in %s: %s (%s)\n", what, cudaGetErrorName(err),
                    cudaGetErrorString(err));
        return false;
    }
    return true;
}

int main()
{
    int count = 0;
    if (!check(cudaGetDeviceCount(&count), "cudaGetDeviceCount")) {
        return 1;
    }
    for (int dev = 0; dev < count; ++dev) {
        cudaDeviceProp prop{};
        int gdr = 0;
        if (!check(cudaGetDeviceProperties(&prop, dev), "cudaGetDeviceProperties") ||
            !check(cudaDeviceGetAttribute(&gdr, cudaDevAttrGPUDirectRDMASupported, dev),
                   "cudaDeviceGetAttribute")) {
            return 1;
        }
        std::printf("GPU %d (%s, PCI bus %d device %d): GPUDirect RDMA APIs supported = %d\n",
                    dev, prop.name, prop.pciBusID, prop.pciDeviceID, gdr);
    }
    return 0;
}
