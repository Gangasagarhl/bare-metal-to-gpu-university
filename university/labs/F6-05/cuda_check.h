// F6-05 Listing 1: the error-checking helpers used by every later CU201 listing.
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

inline void cudaCheck(cudaError_t err, const char* call, const char* file, int line)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s:%d: %s failed: %s (%s)\n", file, line, call,
                     cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

// Wrap every runtime call: CUDA_CHECK(cudaMalloc(&p, bytes));
#define CUDA_CHECK(call) cudaCheck((call), #call, __FILE__, __LINE__)

// After every launch. A launch returns nothing, so ask for the last error; in a
// debug build also wait for the kernel, so that errors inside it surface here.
#ifdef CU201_DEBUG_SYNC
#define CUDA_CHECK_LAUNCH()                        \
    do {                                           \
        CUDA_CHECK(cudaGetLastError());            \
        CUDA_CHECK(cudaDeviceSynchronize());       \
    } while (0)
#else
#define CUDA_CHECK_LAUNCH() CUDA_CHECK(cudaGetLastError())
#endif
