// BR-01 Listing 7: a CPU model of one launch, to see the bounds-check trap without a GPU.
// This is the university's own teaching model, not CUDA: two loops play the grid, and
// AddressSanitizer plays the role that Compute Sanitizer plays on a real GPU.
#include <cstddef>
#include <cstdio>
#include <vector>

// The body of vecAdd, with the three built-in values passed in as ordinary arguments.
void vecAddBody(unsigned blockIdx, unsigned blockDim, unsigned threadIdx, const float* a,
                const float* b, float* c, std::size_t n, bool boundsCheck)
{
    const std::size_t i = blockIdx * static_cast<std::size_t>(blockDim) + threadIdx;
    if (boundsCheck && i >= n) {
        return;
    }
    c[i] = a[i] + b[i];
}

void launchModel(unsigned blocks, unsigned threadsPerBlock, const float* a, const float* b,
                 float* c, std::size_t n, bool boundsCheck)
{
    for (unsigned blk = 0; blk < blocks; ++blk) {           // every block of the grid ...
        for (unsigned t = 0; t < threadsPerBlock; ++t) {    // ... every thread of the block
            vecAddBody(blk, threadsPerBlock, t, a, b, c, n, boundsCheck);
        }
    }
}

int main()
{
    const std::size_t n = 1000;
    const unsigned threadsPerBlock = 256;
    const unsigned blocks = static_cast<unsigned>((n + threadsPerBlock - 1) / threadsPerBlock);
    std::vector<float> a(n, 1.0f), b(n, 2.0f), c(n, 0.0f);
    std::printf("n = %zu, blocks = %u, threads = %u, extra threads = %zu\n", n, blocks,
                blocks * threadsPerBlock, blocks * threadsPerBlock - n);

    launchModel(blocks, threadsPerBlock, a.data(), b.data(), c.data(), n, true);
    std::printf("with the bounds check:    c[999] = %.1f, no out-of-range access\n", c[999]);
    std::fflush(stdout);

    std::printf("without the bounds check: (AddressSanitizer's report follows)\n");
    std::fflush(stdout);
    launchModel(blocks, threadsPerBlock, a.data(), b.data(), c.data(), n, false);
    std::printf("not reached\n");
    return 0;
}
