// BR-04 Listing 7: "time is dominated by communication unless overlapped". A data-parallel
// training step on N GPUs: the compute of one step on one GPU (T1) is split N ways (fixed
// global batch), then the gradient (G bytes) is all-reduced with a ring. The ring uses the
// slowest link it must cross: inside one TN-8 node (N <= 8) the crossing path of Listing 2,
// across machines an invented network. Printed: step time without overlap (compute, then
// communication) and the best case with perfect overlap, max(compute, communication).
// Every value is an invented teaching value, not a measurement and not a real product.
#include <algorithm>
#include <cstdio>

namespace {

double ringUs(int n, double bytes, double alphaUs, double betaGBps)
{
    return n == 1 ? 0.0 : 2.0 * (n - 1) * (alphaUs + bytes / n / (betaGBps * 1e3));
}

}  // namespace

int main()
{
    const double t1Us = 800000.0;                               // compute of one step on one GPU: 800 ms
    const double grad = 256.0 * 1024 * 1024;                    // gradient: 256 MiB
    std::printf("invented: T1 = %.0f ms compute per step on 1 GPU, gradient %.0f MiB\n", t1Us / 1e3,
                grad / 1048576.0);
    std::printf("links: N <= 8 one TN-8 node (bottleneck alpha 10 us, beta 10 GB/s); "
                "N > 8 several nodes (bottleneck network alpha 20 us, beta 5 GB/s)\n");
    std::printf("%4s %12s %12s %14s %13s %15s %13s\n", "N", "compute ms", "comm ms", "no overlap ms",
                "comm share", "full overlap ms", "speed-up");
    for (int n = 1; n <= 64; n *= 2) {
        const double compute = t1Us / n;
        const bool oneNode = n <= 8;
        const double comm = ringUs(n, grad, oneNode ? 10.0 : 20.0, oneNode ? 10.0 : 5.0);
        const double serial = compute + comm;
        const double overlapped = std::max(compute, comm);
        std::printf("%4d %12.1f %12.1f %14.1f %12.0f%% %15.1f %6.2f/%-6.2f\n", n, compute / 1e3, comm / 1e3,
                    serial / 1e3, 100.0 * comm / serial, overlapped / 1e3, t1Us / serial, t1Us / overlapped);
    }
    std::printf("speed-up column: without overlap / with full overlap, relative to 1 GPU\n");
    return 0;
}
