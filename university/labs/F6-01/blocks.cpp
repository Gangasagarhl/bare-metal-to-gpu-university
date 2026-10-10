// F6-01 Listing 2: the worked example's arithmetic, checked by a program.
// How many blocks does a launch need, how many threads do nothing, and which
// element does a given thread handle?
#include <cstdio>

int blocksFor(int n, int threadsPerBlock)
{
    return (n + threadsPerBlock - 1) / threadsPerBlock;  // round up
}

int globalIndex(int block, int threadsPerBlock, int thread)
{
    return block * threadsPerBlock + thread;  // blockIdx.x * blockDim.x + threadIdx.x
}

void report(int n, int threadsPerBlock)
{
    const int blocks = blocksFor(n, threadsPerBlock);
    const int threads = blocks * threadsPerBlock;
    std::printf("n = %d, %d threads per block: %d blocks, %d threads, %d do nothing\n",
                n, threadsPerBlock, blocks, threads, threads - n);
}

void who(int block, int thread, int threadsPerBlock, int n)
{
    const int i = globalIndex(block, threadsPerBlock, thread);
    std::printf("block %d, thread %d -> i = %d * %d + %d = %d: %s\n", block, thread, block,
                threadsPerBlock, thread, i, i < n ? "adds this element" : "bounds check stops it");
}

int main()
{
    report(1000, 256);
    who(2, 3, 256, 1000);
    who(3, 255, 256, 1000);
    report(300, 128);
    report(1 << 20, 256);
    return 0;
}
