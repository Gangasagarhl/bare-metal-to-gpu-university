// F6-31 Listing 3: a cost model of managed memory on TG-1. ALL NUMBERS INVENTED:
//   link 16 GB/s with 10 us per bulk copy (TG-1);
//   on-demand migration moves data in groups of groupKiB per GPU page fault and
//   pays faultUs of fault handling per group (TG-1 extension, invented);
// Compares, for a kernel that touches every byte of a buffer once per iteration:
//   explicit   cudaMemcpy-style bulk copy in and out once
//   prefetch   bulk migration in before the first kernel, out after the last
//   on-demand  page faults on the GPU, then on the CPU when the host reads
//   pingpong   the CPU reads the whole buffer after EVERY iteration (data moves
//              back and forth each time)
// Input lines: label MiB iterations kernelMsPerIteration groupKiB faultUs
#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    const double linkBytesPerMs = 16.0e9 / 1000.0;
    const double bulkFixedMs = 0.010;
    std::string label;
    double mib = 0, kMs = 0, groupKiB = 0, faultUs = 0;
    int iters = 0;
    while (std::cin >> label >> mib >> iters >> kMs >> groupKiB >> faultUs) {
        const double bytes = mib * 1024.0 * 1024.0;
        const double groups = bytes / (groupKiB * 1024.0);
        const double bulk = bulkFixedMs + bytes / linkBytesPerMs;            // one direction
        const double demandOneWay = groups * faultUs / 1000.0 + bytes / linkBytesPerMs;
        const double compute = iters * kMs;
        const double explicitMs = 2.0 * bulk + compute;
        const double prefetchMs = 2.0 * bulk + compute;
        const double demandMs = 2.0 * demandOneWay + compute;
        const double pingpongMs = iters * 2.0 * demandOneWay + compute;
        std::printf("%s: %.0f MiB, %d iterations, kernel %.2f ms each, %.0f fault groups\n",
                    label.c_str(), mib, iters, kMs, groups);
        std::printf("  explicit copies %9.3f ms\n", explicitMs);
        std::printf("  prefetch        %9.3f ms\n", prefetchMs);
        std::printf("  on-demand       %9.3f ms  (%.2fx explicit)\n", demandMs,
                    demandMs / explicitMs);
        std::printf("  ping-pong       %9.3f ms  (%.2fx explicit)\n", pingpongMs,
                    pingpongMs / explicitMs);
    }
    return 0;
}
