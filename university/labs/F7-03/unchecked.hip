// unchecked.hip - the same kind of program with every result ignored.
// HIP declares hipError_t [[nodiscard]], so the compiler warns; the run shows why it should.
#include <hip/hip_runtime.h>
#include <cstdio>
#include <vector>

__global__ void setOnes(int* x, int n)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        x[i] = 1;
    }
}

int main()
{
    const int n = 256;
    std::vector<int> hX(n, 0);
    int* dX = nullptr;
    hipMalloc(&dX, n * sizeof(int));                                     // result ignored
    setOnes<<<1, n>>>(dX, n);                                            // launch not checked
    hipMemcpy(hX.data(), dX, n * sizeof(int), hipMemcpyDeviceToHost);   // result ignored
    int sum = 0;
    for (int v : hX) {
        sum += v;
    }
    std::printf("sum of %d ones = %d\n", n, sum);
    hipFree(dX);                                                         // result ignored
    return 0;
}
