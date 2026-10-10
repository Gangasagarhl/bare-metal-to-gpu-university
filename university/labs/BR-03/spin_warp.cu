// BR-03 Listing 4: the CPU spin lock (F2-38) carried to the GPU, and two safe alternatives.
//   countSpinLock  CPU habit: spin until the lock is ours, increment, unlock (after the loop)
//   countWarpSafe  the increment and the unlock are inside the loop, by the lane that won
//   countAtomic    no lock at all: one atomic add per thread
// Built for real in this build; untested on hardware (no GPU).
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

__device__ int gLock = 0;           // 0 = free, 1 = taken
__device__ unsigned gCounter = 0;   // protected by gLock in the first two kernels

__global__ void countSpinLock()
{
    while (atomicCAS(&gLock, 0, 1) != 0) {   // spin until we own the lock
    }
    volatile unsigned* c = &gCounter;
    *c = *c + 1;                              // critical section
    __threadfence();                          // make the write visible before the unlock
    atomicExch(&gLock, 0);                    // unlock
}

__global__ void countWarpSafe()
{
    bool done = false;
    while (!done) {
        if (atomicCAS(&gLock, 0, 1) == 0) {   // this lane won the lock ...
            volatile unsigned* c = &gCounter;
            *c = *c + 1;                      // ... does the work ...
            __threadfence();
            atomicExch(&gLock, 0);            // ... and releases it, all inside the loop
            done = true;
        }
    }
}

__global__ void countAtomic()
{
    atomicAdd(&gCounter, 1u);
}

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const char* names[] = {"countAtomic", "countWarpSafe", "countSpinLock"};
    for (int k = 0; k < 3; ++k) {
        const unsigned zero = 0;
        check(cudaMemcpyToSymbol(gCounter, &zero, sizeof zero), "reset counter");
        if (k == 0) { countAtomic<<<4, 64>>>(); }
        if (k == 1) { countWarpSafe<<<4, 64>>>(); }
        if (k == 2) { countSpinLock<<<4, 64>>>(); }  // may never finish on GPUs without
                                                     // independent thread scheduling
        check(cudaGetLastError(), names[k]);
        check(cudaDeviceSynchronize(), names[k]);
        unsigned got = 0;
        check(cudaMemcpyFromSymbol(&got, gCounter, sizeof got), "read counter");
        std::printf("%-14s counter = %u (expected 256)\n", names[k], got);
    }
    return 0;
}
