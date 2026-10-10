// BR-01 break 1 (does not compile, on purpose): Listing 5 with the specifiers removed from add().
#include <cstddef>
#include <cstdio>
#include <exception>
#include <vector>
#include "br01_check.h"

template <typename T>
T add(T a, T b)                                    // BREAK 1: no __host__ __device__
{
    return a + b;
}

template <typename T>
__global__ void vecAdd(const T* a, const T* b, T* c, std::size_t n)
{
    const std::size_t i = blockIdx.x * static_cast<std::size_t>(blockDim.x) + threadIdx.x;
    if (i < n) {                                    // the bounds check
        c[i] = add(a[i], b[i]);
    }
}

void vecAddCpu(const std::vector<float>& a, const std::vector<float>& b, std::vector<float>& c)
{
    for (std::size_t i = 0; i < c.size(); ++i) {
        c[i] = add(a[i], b[i]);                     // the same add(), now as the reference
    }
}

int main()
{
    try {
        const std::size_t n = 1000003;
        std::vector<float> hA(n), hB(n), hC(n, -1.0f), ref(n);
        for (std::size_t i = 0; i < n; ++i) {
            hA[i] = 1.0f * static_cast<float>(i);
            hB[i] = 2.0f * static_cast<float>(i);
        }
        vecAddCpu(hA, hB, ref);

        DeviceBuffer<float> dA(n), dB(n), dC(n);
        dA.copyFromHost(hA);
        dB.copyFromHost(hB);

        const unsigned threadsPerBlock = 256;
        const unsigned blocks = static_cast<unsigned>((n + threadsPerBlock - 1) / threadsPerBlock);
        vecAdd<<<blocks, threadsPerBlock>>>(dA.get(), dB.get(), dC.get(), n);
        CUDA_CHECK(cudaGetLastError());             // was the launch accepted?
        CUDA_CHECK(cudaDeviceSynchronize());        // wait; report errors from inside the kernel

        dC.copyToHost(hC);
        std::size_t errors = 0;
        for (std::size_t i = 0; i < n; ++i) {
            if (hC[i] != ref[i]) {
                ++errors;
            }
        }
        std::printf("blocks = %u, threads = %u, n = %zu, errors = %zu\n",
                    blocks, threadsPerBlock, n, errors);
        return errors == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
