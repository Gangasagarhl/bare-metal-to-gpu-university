// F6-05 forensic evidence: "the kernel that silently did nothing" (deliberately broken).
// Kofi wanted bigger blocks "for speed" and wrote 2048 threads per block.
#include <cstdio>
#include <vector>
#include <cuda_runtime.h>

__global__ void square(const float* in, float* out, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        out[i] = in[i] * in[i];
    }
}

int main()
{
    const int n = 1 << 16;
    std::vector<float> hIn(n), hOut(n);
    for (int i = 0; i < n; ++i) { hIn[i] = 0.5f * i; }
    float* dIn = nullptr;
    float* dOut = nullptr;
    cudaMalloc(&dIn, n * sizeof(float));
    cudaMalloc(&dOut, n * sizeof(float));
    cudaMemcpy(dIn, hIn.data(), n * sizeof(float), cudaMemcpyHostToDevice);
    const int threads = 2048;                       // the change made "for speed"
    square<<<(n + threads - 1) / threads, threads>>>(dIn, dOut, n);
    cudaMemcpy(hOut.data(), dOut, n * sizeof(float), cudaMemcpyDeviceToHost);
    double sum = 0.0;
    for (int i = 0; i < n; ++i) { sum += hOut[i]; }
    std::printf("out[1] = %.2f, out[2] = %.2f, sum of all outputs = %.2f\n", hOut[1], hOut[2], sum);
    cudaFree(dIn);
    cudaFree(dOut);
    return 0;
}
