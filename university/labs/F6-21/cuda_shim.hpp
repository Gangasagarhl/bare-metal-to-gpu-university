// cuda_shim.hpp: runs the CU302 GEMM kernels on the CPU, unchanged, for correctness tests.
// This is the university's own code, not NVIDIA's. It emulates only the small subset of
// CUDA C++ that the kernels use: __global__/__device__, threadIdx, blockIdx, blockDim,
// gridDim, __shared__, __syncthreads(), float4, and the cp.async primitives of F6-24.
// Each block runs as blockDim threads of std::thread joined by a std::barrier, so a missing
// __syncthreads() really races and a thread that skips a barrier really hangs.
// Blocks run one after another, so a function-local static array is the block's shared memory.
// It says nothing about speed: timing needs a GPU.
#pragma once
// GCC does not know the CUDA "#pragma unroll" hint; on the CPU it changes nothing.
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <barrier>
#include <cstddef>
#include <cstring>
#include <thread>
#include <vector>

#define __global__
#define __device__
#define __host__
#define __forceinline__ inline
#define __shared__ static
#define __launch_bounds__(...)

struct dim3
{
    unsigned x = 1, y = 1, z = 1;
    constexpr dim3(unsigned x_ = 1, unsigned y_ = 1, unsigned z_ = 1) : x(x_), y(y_), z(z_) {}
};

struct alignas(16) float4
{
    float x, y, z, w;
};

namespace shim {
inline thread_local dim3 tIdx;
inline thread_local dim3 bIdx;
inline dim3 bDim;
inline dim3 gDim;
inline thread_local std::barrier<>* blockBarrier = nullptr;

// cp.async model (F6-24): a copy is queued and lands only when the thread waits for its
// group, the latest moment the real primitive allows. Reading too early sees stale data.
struct PendingCopy
{
    void* dst;
    const void* src;
    std::size_t bytes;
};
inline thread_local std::vector<std::vector<PendingCopy>> committedGroups;
inline thread_local std::vector<PendingCopy> openGroup;
}  // namespace shim

#define threadIdx (shim::tIdx)
#define blockIdx (shim::bIdx)
#define blockDim (shim::bDim)
#define gridDim (shim::gDim)

inline void __syncthreads()
{
    shim::blockBarrier->arrive_and_wait();
}

inline void __pipeline_memcpy_async(void* dst, const void* src, std::size_t bytes)
{
    shim::openGroup.push_back({dst, src, bytes});
}

inline void __pipeline_commit()
{
    shim::committedGroups.push_back(shim::openGroup);
    shim::openGroup.clear();
}

inline void __pipeline_wait_prior(std::size_t prior)
{
    auto& g = shim::committedGroups;
    while (g.size() > prior) {                    // land the oldest groups, keep `prior` pending
        for (const auto& c : g.front()) {
            std::memcpy(c.dst, c.src, c.bytes);
        }
        g.erase(g.begin());
    }
}

// launch(grid, block, kernel, args...) plays the role of kernel<<<grid, block>>>(args...).
template <typename Kernel, typename... Args>
void launch(dim3 grid, dim3 block, Kernel kernel, Args... args)
{
    shim::gDim = grid;
    shim::bDim = block;
    const unsigned n = block.x * block.y * block.z;
    for (unsigned bz = 0; bz < grid.z; ++bz) {
        for (unsigned by = 0; by < grid.y; ++by) {
            for (unsigned bx = 0; bx < grid.x; ++bx) {
                std::barrier<> bar(static_cast<std::ptrdiff_t>(n));
                std::vector<std::thread> threads;
                threads.reserve(n);
                for (unsigned t = 0; t < n; ++t) {
                    threads.emplace_back([&, t] {
                        shim::bIdx = dim3(bx, by, bz);
                        shim::tIdx = dim3(t % block.x, (t / block.x) % block.y, t / (block.x * block.y));
                        shim::blockBarrier = &bar;
                        kernel(args...);
                        __pipeline_wait_prior(0);  // a real thread's copies also land eventually
                    });
                }
                for (std::thread& th : threads) {
                    th.join();
                }
            }
        }
    }
}
