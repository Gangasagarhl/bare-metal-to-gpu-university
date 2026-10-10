// BR-01 Listing 10: the Worked example's arithmetic, checked on the CPU.
#include <cstddef>
#include <cstdio>

int main()
{
    const std::size_t n = 1000003;
    const unsigned threadsPerBlock = 256;
    const unsigned blocks = static_cast<unsigned>((n + threadsPerBlock - 1) / threadsPerBlock);
    const std::size_t threads = static_cast<std::size_t>(blocks) * threadsPerBlock;
    std::printf("n = %zu, threadsPerBlock = %u\n", n, threadsPerBlock);
    std::printf("blocks = %u, threads in the grid = %zu, threads with no element = %zu\n",
                blocks, threads, threads - n);

    const std::size_t last = n - 1;                 // which thread adds the last element?
    std::printf("element %zu: block %zu, thread %zu\n", last, last / threadsPerBlock,
                last % threadsPerBlock);

    const std::size_t bytes = n * sizeof(float);
    std::printf("bytes per buffer = %zu, three device buffers = %zu bytes\n", bytes, 3 * bytes);

    // Why Listing 5 widens to std::size_t before multiplying: blockIdx.x and blockDim.x
    // are unsigned int, and an unsigned int product wraps around.
    const unsigned bigBlock = 16777216;             // 2 to the power 24
    const unsigned narrow = bigBlock * threadsPerBlock;
    const std::size_t wide = bigBlock * static_cast<std::size_t>(threadsPerBlock);
    std::printf("block %u, thread 0: unsigned int index = %u, std::size_t index = %zu\n",
                bigBlock, narrow, wide);
    return 0;
}
