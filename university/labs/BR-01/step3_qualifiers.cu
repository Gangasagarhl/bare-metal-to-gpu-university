// BR-01 Listing 2: the same program as a .cu file built by nvcc, with execution-space
// specifiers added. Nothing runs on a GPU yet: every line that executes here runs on the CPU.
#include <cstddef>
#include <cstdio>
#include <vector>

template <typename T>
__host__ __device__ T add(T a, T b)                 // compiled for the CPU and for the GPU
{
    return a + b;
}

__host__ __device__ const char* compiledFor()
{
#ifdef __CUDA_ARCH__
    return "device";                                // only in the device compilation pass
#else
    return "host";                                  // only in the host compilation pass
#endif
}

template <typename T>
__global__ void vecAdd(const T* a, const T* b, T* c, std::size_t n)  // written, not launched yet
{
    const std::size_t i = blockIdx.x * static_cast<std::size_t>(blockDim.x) + threadIdx.x;
    if (i < n) {
        c[i] = add(a[i], b[i]);
    }
}

void vecAddCpu(const std::vector<float>& a, const std::vector<float>& b, std::vector<float>& c)
{
    for (std::size_t i = 0; i < c.size(); ++i) {
        c[i] = add(a[i], b[i]);
    }
}

int main()
{
    const std::size_t n = 1000003;
    std::vector<float> a(n), b(n), c(n);
    for (std::size_t i = 0; i < n; ++i) {
        a[i] = 1.0f * static_cast<float>(i);
        b[i] = 2.0f * static_cast<float>(i);
    }
    vecAddCpu(a, b, c);
    std::size_t errors = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (c[i] != 3.0f * static_cast<float>(i)) {
            ++errors;
        }
    }
    std::printf("n = %zu, c[1] = %.1f, c[n-1] = %.1f, errors = %zu\n", n, c[1], c[n - 1], errors);
    std::printf("main() was compiled in the %s pass; add(2, 3) = %d, add(1.5f, 2.0f) = %.1f\n",
                compiledFor(), add(2, 3), add(1.5f, 2.0f));
    return errors == 0 ? 0 : 1;
}
