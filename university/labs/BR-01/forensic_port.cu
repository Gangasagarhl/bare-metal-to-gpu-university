// BR-01 forensic evidence: Kwame's first port of the vector-add program (does not compile).
// Kwame's note: "I only changed the loop into a kernel. Why does it not build?"
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <cuda_runtime.h>

float addPair(float a, float b)
{
    return a + b;
}

__global__ void vecAdd(const std::vector<float>& a, const std::vector<float>& b,
                       std::vector<float>& c)
{
    const std::size_t i = blockIdx.x * static_cast<std::size_t>(blockDim.x) + threadIdx.x;
    if (i >= c.size()) {
        throw std::out_of_range("index past the end");
    }
    c[i] = addPair(a[i], b[i]);
}

int main()
{
    const std::size_t n = 1000003;
    std::vector<float> a(n, 1.0f), b(n, 2.0f), c(n);
    vecAdd(a, b, c);
    std::printf("c[1] = %.1f\n", c[1]);
    return 0;
}
