// saxpy_kernel.hip - GPU code and its HIP runtime calls, compiled as HIP.
#include "saxpy.h"
#include <hip/hip_runtime.h>

__global__ void saxpy(int n, float a, const float* x, float* y)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = a * x[i] + y[i];
    }
}

const char* saxpyOnGpu(float a, const std::vector<float>& x, std::vector<float>& y)
{
    const int n = static_cast<int>(x.size());
    const size_t bytes = x.size() * sizeof(float);
    float* dX = nullptr;
    float* dY = nullptr;
    // stop at the first failing call; its error is the one reported
    hipError_t err = hipMalloc(&dX, bytes);
    if (err == hipSuccess) {
        err = hipMalloc(&dY, bytes);
    }
    if (err == hipSuccess) {
        err = hipMemcpy(dX, x.data(), bytes, hipMemcpyHostToDevice);
    }
    if (err == hipSuccess) {
        err = hipMemcpy(dY, y.data(), bytes, hipMemcpyHostToDevice);
    }
    if (err == hipSuccess) {
        saxpy<<<(n + 255) / 256, 256>>>(n, a, dX, dY);
        err = hipGetLastError();
    }
    if (err == hipSuccess) {
        err = hipMemcpy(y.data(), dY, bytes, hipMemcpyDeviceToHost);
    }
    if (dX != nullptr) {
        (void)hipFree(dX);
    }
    if (dY != nullptr) {
        (void)hipFree(dY);
    }
    return err == hipSuccess ? "" : hipGetErrorString(err);
}
