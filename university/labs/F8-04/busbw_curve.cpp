// F8-04 forensic evidence: the bus-bandwidth curve of the two 4-GPU placements on TN-1,
// from the ring model of F8-03, with each placement's ring bandwidth from Listing 1
// (25 GB/s for GPUs 0,1,2,3; 12.5 GB/s for GPUs 0,1,4,5) and the invented alpha of 10 us.
#include <cstdio>

int main()
{
    const int n = 4;
    const double alphaUs = 10.0;
    const double ringGBps[2] = {25.0, 12.5};
    std::printf("ring all-reduce on 4 GPUs of TN-1 (model, invented link values)\n");
    std::printf("%12s %18s %18s\n", "bytes", "busbw GPUs 0-3", "busbw GPUs 0,1,4,5");
    for (double s = 1024; s <= 1024.0 * 1024 * 1024; s *= 8) {
        std::printf("%12.0f", s);
        for (double beta : ringGBps) {
            const double us = 2.0 * (n - 1) * (alphaUs + (s / n) / (beta * 1e3));
            const double busbw = s / us / 1e3 * 2.0 * (n - 1) / n;
            std::printf(" %18.3f", busbw);
        }
        std::printf("\n");
    }
    return 0;
}
