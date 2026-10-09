// F1-55 forensic evidence: a time-stepping loop moved to the GPU as it was on the CPU.
// One thread does every step; each step needs the previous one. Untested on hardware.
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

__global__ void integrate(float* state, int steps)
{
    float x = state[0];
    for (int s = 0; s < steps; ++s) {
        x = x * 0.999f + 0.001f;      // step s needs the result of step s - 1
    }
    state[0] = x;
}

int main()
{
    float* dState = nullptr;
    cudaError_t err = cudaMalloc(&dState, sizeof(float));
    if (err != cudaSuccess) {
        std::fprintf(stderr, "cudaMalloc failed: %s (%s)\n", cudaGetErrorName(err), cudaGetErrorString(err));
        return EXIT_FAILURE;
    }
    integrate<<<1, 1>>>(dState, 100000000);   // one block of one thread
    err = cudaDeviceSynchronize();
    std::printf("integrate finished: %s\n", cudaGetErrorString(err));
    cudaFree(dState);
    return err == cudaSuccess ? 0 : 1;
}
