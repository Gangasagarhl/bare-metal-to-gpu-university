// F8-04 Listing 2: the milestone F1 tool. For every ordered GPU pair it prints whether peer
// access is possible, the link attributes the runtime reports, and the measured peer copy
// bandwidth, one direction and both directions at once (median of 9 timed copies of 64 MiB).
// Built for real with nvcc; untested on hardware (the build container has no GPU).
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

// median time in ms of `reps` copies src(dev a) -> dst(dev b); if `both`, b -> a runs at once
static float timeCopies(int a, int b, void* bufA, void* bufB, void* bufA2, void* bufB2,
                        std::size_t bytes, bool both)
{
    cudaStream_t sa = nullptr, sb = nullptr;
    cudaEvent_t start = nullptr, stop = nullptr, otherDone = nullptr;
    check(cudaSetDevice(b), "cudaSetDevice b");
    check(cudaStreamCreate(&sb), "cudaStreamCreate b");
    check(cudaEventCreateWithFlags(&otherDone, cudaEventDisableTiming), "cudaEventCreate otherDone");
    check(cudaSetDevice(a), "cudaSetDevice a");
    check(cudaStreamCreate(&sa), "cudaStreamCreate a");
    check(cudaEventCreate(&start), "cudaEventCreate start");
    check(cudaEventCreate(&stop), "cudaEventCreate stop");
    std::vector<float> ms;
    for (int rep = 0; rep < 10; ++rep) {                       // the first one is a warm-up
        check(cudaEventRecord(start, sa), "cudaEventRecord start");
        check(cudaMemcpyPeerAsync(bufB, b, bufA, a, bytes, sa), "cudaMemcpyPeerAsync a->b");
        if (both) {
            check(cudaStreamWaitEvent(sb, start, 0), "cudaStreamWaitEvent");
            check(cudaMemcpyPeerAsync(bufA2, a, bufB2, b, bytes, sb), "cudaMemcpyPeerAsync b->a");
            check(cudaEventRecord(otherDone, sb), "cudaEventRecord otherDone");
            check(cudaStreamWaitEvent(sa, otherDone, 0), "cudaStreamWaitEvent otherDone");
        }
        check(cudaEventRecord(stop, sa), "cudaEventRecord stop");
        check(cudaEventSynchronize(stop), "cudaEventSynchronize");
        float t = 0.0f;
        check(cudaEventElapsedTime(&t, start, stop), "cudaEventElapsedTime");
        if (rep > 0) {
            ms.push_back(t);
        }
    }
    std::sort(ms.begin(), ms.end());
    check(cudaEventDestroy(start), "cudaEventDestroy");
    check(cudaEventDestroy(stop), "cudaEventDestroy");
    check(cudaEventDestroy(otherDone), "cudaEventDestroy");
    check(cudaStreamDestroy(sa), "cudaStreamDestroy");
    check(cudaStreamDestroy(sb), "cudaStreamDestroy");
    return ms[ms.size() / 2];
}

int main()
{
    int n = 0;
    check(cudaGetDeviceCount(&n), "cudaGetDeviceCount");
    std::printf("%d GPU(s)\n", n);
    for (int d = 0; d < n; ++d) {
        cudaDeviceProp p{};
        check(cudaGetDeviceProperties(&p, d), "cudaGetDeviceProperties");
        std::printf("GPU %d: %s, PCI %04x:%02x:%02x\n", d, p.name, p.pciDomainID, p.pciBusID, p.pciDeviceID);
    }
    std::printf("\npair   canAccess  perfRank  atomics\n");
    for (int a = 0; a < n; ++a) {
        for (int b = 0; b < n; ++b) {
            if (a == b) {
                continue;
            }
            int can = 0, rank = 0, atomics = 0;
            check(cudaDeviceCanAccessPeer(&can, a, b), "cudaDeviceCanAccessPeer");
            check(cudaDeviceGetP2PAttribute(&rank, cudaDevP2PAttrPerformanceRank, a, b), "P2P perf rank");
            check(cudaDeviceGetP2PAttribute(&atomics, cudaDevP2PAttrNativeAtomicSupported, a, b), "P2P atomics");
            std::printf("%d->%d  %9d %9d %8d\n", a, b, can, rank, atomics);
            if (can) {                                         // enable a -> b access once
                check(cudaSetDevice(a), "cudaSetDevice");
                const cudaError_t e = cudaDeviceEnablePeerAccess(b, 0);
                if (e != cudaErrorPeerAccessAlreadyEnabled) {
                    check(e, "cudaDeviceEnablePeerAccess");
                }
                (void)cudaGetLastError();                      // clear an "already enabled" error
            }
        }
    }
    const std::size_t bytes = 64u << 20;
    std::vector<void*> buf(static_cast<std::size_t>(n)), buf2(static_cast<std::size_t>(n));
    for (int d = 0; d < n; ++d) {
        check(cudaSetDevice(d), "cudaSetDevice");
        check(cudaMalloc(&buf[static_cast<std::size_t>(d)], bytes), "cudaMalloc");
        check(cudaMalloc(&buf2[static_cast<std::size_t>(d)], bytes), "cudaMalloc");
    }
    for (int both = 0; both <= 1; ++both) {
        std::printf("\n%s peer copy bandwidth, GB/s (row = source, column = destination)\n",
                    both ? "bidirectional (sum of both directions)" : "unidirectional");
        for (int a = 0; a < n; ++a) {
            std::printf("GPU%d", a);
            for (int b = 0; b < n; ++b) {
                if (a == b) {
                    std::printf("%9s", "-");
                    continue;
                }
                const auto ia = static_cast<std::size_t>(a), ib = static_cast<std::size_t>(b);
                const float ms = timeCopies(a, b, buf[ia], buf[ib], buf2[ia], buf2[ib], bytes, both != 0);
                const double moved = static_cast<double>(bytes) * (both ? 2.0 : 1.0);
                std::printf("%9.2f", moved / (ms * 1e-3) / 1e9);
            }
            std::printf("\n");
        }
    }
    for (int d = 0; d < n; ++d) {
        check(cudaSetDevice(d), "cudaSetDevice");
        check(cudaFree(buf[static_cast<std::size_t>(d)]), "cudaFree");
        check(cudaFree(buf2[static_cast<std::size_t>(d)]), "cudaFree");
    }
    return 0;
}
