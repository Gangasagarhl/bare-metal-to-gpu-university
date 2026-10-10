// F6-37 Listing 1: the peer bandwidth and latency matrix of curriculum milestone F1.
// For every ordered pair of GPUs (src, dst): can src's memory be accessed by dst? Enable peer
// access where possible, then time cudaMemcpyPeerAsync with events: median of 20 runs after
// warm-up, one direction at a time and both directions at once.
// Untested on hardware: the build container has no GPU.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

constexpr int kRuns = 20;

// median time in ms of copying `bytes` from src to dst (and dst to src too if both)
float timeCopies(int src, int dst, void* bufSrc, void* bufDst, void* bufSrc2, void* bufDst2,
                 std::size_t bytes, bool both)
{
    cudaStream_t sA = nullptr;
    cudaStream_t sB = nullptr;
    cudaEvent_t start = nullptr;
    cudaEvent_t stop = nullptr;
    check(cudaSetDevice(src), "cudaSetDevice src");
    check(cudaStreamCreateWithFlags(&sA, cudaStreamNonBlocking), "stream A");
    check(cudaEventCreate(&start), "event start");
    check(cudaEventCreate(&stop), "event stop");
    check(cudaSetDevice(dst), "cudaSetDevice dst");
    check(cudaStreamCreateWithFlags(&sB, cudaStreamNonBlocking), "stream B");
    std::vector<float> ms;
    for (int r = -3; r < kRuns; ++r) {                    // three warm-up runs, then timed runs
        check(cudaSetDevice(src), "cudaSetDevice src");
        check(cudaEventRecord(start, sA), "record start");
        check(cudaMemcpyPeerAsync(bufDst, dst, bufSrc, src, bytes, sA), "copy src->dst");
        if (both) {
            check(cudaStreamWaitEvent(sB, start, 0), "B waits for start");
            check(cudaMemcpyPeerAsync(bufSrc2, src, bufDst2, dst, bytes, sB), "copy dst->src");
            cudaEvent_t doneB = nullptr;
            check(cudaSetDevice(dst), "cudaSetDevice dst");
            check(cudaEventCreate(&doneB), "event doneB");
            check(cudaEventRecord(doneB, sB), "record doneB");
            check(cudaStreamWaitEvent(sA, doneB, 0), "A waits for B");
            check(cudaEventDestroy(doneB), "destroy doneB");
            check(cudaSetDevice(src), "cudaSetDevice src");
        }
        check(cudaEventRecord(stop, sA), "record stop");
        check(cudaEventSynchronize(stop), "sync stop");
        float t = 0.0f;
        check(cudaEventElapsedTime(&t, start, stop), "elapsed");
        if (r >= 0) {
            ms.push_back(t);
        }
    }
    std::sort(ms.begin(), ms.end());
    check(cudaEventDestroy(start), "destroy start");
    check(cudaEventDestroy(stop), "destroy stop");
    check(cudaStreamDestroy(sA), "destroy A");
    check(cudaStreamDestroy(sB), "destroy B");
    return 0.5f * (ms[kRuns / 2 - 1] + ms[kRuns / 2]);
}

int main()
{
    int count = 0;
    check(cudaGetDeviceCount(&count), "cudaGetDeviceCount");
    std::printf("%d GPU(s)\n", count);
    if (count < 2) {
        std::printf("the matrix needs at least two GPUs\n");
        return EXIT_FAILURE;
    }
    std::printf("access matrix (row = device that reads, column = device that owns the memory), "
                "performance rank in brackets (lower is better):\n");
    for (int a = 0; a < count; ++a) {
        for (int b = 0; b < count; ++b) {
            if (a == b) {
                std::printf("     -    ");
                continue;
            }
            int can = 0;
            int rank = 0;
            check(cudaDeviceCanAccessPeer(&can, a, b), "cudaDeviceCanAccessPeer");
            check(cudaDeviceGetP2PAttribute(&rank, cudaDevP2PAttrPerformanceRank, a, b), "P2P rank");
            if (can) {
                check(cudaSetDevice(a), "cudaSetDevice");          // enable on the device that reads
                const cudaError_t e = cudaDeviceEnablePeerAccess(b, 0);
                if (e != cudaSuccess && e != cudaErrorPeerAccessAlreadyEnabled) {
                    check(e, "cudaDeviceEnablePeerAccess");
                }
                cudaGetLastError();                                // clear "already enabled"
            }
            std::printf("  %3s [%d] ", can ? "yes" : "no", rank);
        }
        std::printf("\n");
    }
    const std::size_t sizes[] = {std::size_t{4} << 10, std::size_t{256} << 20};
    std::vector<void*> a(static_cast<std::size_t>(count));
    std::vector<void*> b(static_cast<std::size_t>(count));
    for (int d = 0; d < count; ++d) {
        check(cudaSetDevice(d), "cudaSetDevice");
        check(cudaMalloc(&a[static_cast<std::size_t>(d)], sizes[1]), "cudaMalloc a");
        check(cudaMalloc(&b[static_cast<std::size_t>(d)], sizes[1]), "cudaMalloc b");
    }
    for (std::size_t bytes : sizes) {
        for (int both = 0; both < 2; ++both) {
            std::printf("%s, %zu bytes: %s (rows = source, columns = destination)\n",
                        both ? "bidirectional" : "unidirectional", bytes, bytes < (1u << 20) ? "latency in us" : "GB/s");
            for (int s = 0; s < count; ++s) {
                for (int t = 0; t < count; ++t) {
                    if (s == t) {
                        std::printf("%9s", "-");
                        continue;
                    }
                    const float ms = timeCopies(s, t, a[static_cast<std::size_t>(s)], b[static_cast<std::size_t>(t)],
                                                b[static_cast<std::size_t>(s)], a[static_cast<std::size_t>(t)],
                                                bytes, both != 0);
                    const double moved = static_cast<double>(bytes) * (both ? 2.0 : 1.0);
                    if (bytes < (1u << 20)) {
                        std::printf("%9.1f", static_cast<double>(ms) * 1000.0);
                    } else {
                        std::printf("%9.1f", moved / (static_cast<double>(ms) * 1e-3) / 1e9);
                    }
                }
                std::printf("\n");
            }
        }
    }
    for (int d = 0; d < count; ++d) {
        check(cudaSetDevice(d), "cudaSetDevice");
        check(cudaFree(a[static_cast<std::size_t>(d)]), "cudaFree a");
        check(cudaFree(b[static_cast<std::size_t>(d)]), "cudaFree b");
    }
    return 0;
}
