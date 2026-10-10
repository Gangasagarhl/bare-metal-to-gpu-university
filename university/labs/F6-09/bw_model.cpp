// F6-09 Listing 1: effective bandwidth, the memory roof and the host link, for TG-1
// (the university's invented teaching GPU: 256-bit memory at 8 GT/s; host link 16 GB/s
// with 10 us fixed cost per copy). Input lines:
//   kernel <label> <elements> <bytesReadPerElement> <bytesWrittenPerElement> <ms>
//   copy   <label> <bytes>
#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    const double busBits = 256.0;
    const double transfersPerSecond = 8.0e9;             // 8 GT/s
    const double peak = busBits / 8.0 * transfersPerSecond;  // bytes per second
    const double linkBytesPerSecond = 16.0e9;
    const double linkFixedSeconds = 10.0e-6;
    std::printf("TG-1 memory roof: %.0f bits / 8 x %.0f GT/s = %.1f GB/s\n", busBits,
                transfersPerSecond / 1e9, peak / 1e9);
    std::string kind, label;
    while (std::cin >> kind >> label) {
        if (kind == "kernel") {
            double n = 0, rd = 0, wr = 0, ms = 0;
            std::cin >> n >> rd >> wr >> ms;
            const double bytes = n * (rd + wr);
            const double roofMs = bytes / peak * 1e3;
            const double gbs = bytes / (ms * 1e-3) / 1e9;
            const double gibs = bytes / (ms * 1e-3) / (1024.0 * 1024.0 * 1024.0);
            std::printf("kernel %-16s bytes %12.0f  roof %8.4f ms  measured %8.4f ms  "
                        "%7.1f GB/s = %7.1f GiB/s = %6.1f %% of roof\n",
                        label.c_str(), bytes, roofMs, ms, gbs, gibs, 100.0 * gbs * 1e9 / peak);
        } else if (kind == "copy") {
            double bytes = 0;
            std::cin >> bytes;
            const double seconds = linkFixedSeconds + bytes / linkBytesPerSecond;
            std::printf("copy   %-16s bytes %12.0f  time %10.4f ms  effective %6.2f GB/s\n",
                        label.c_str(), bytes, seconds * 1e3, bytes / seconds / 1e9);
        } else {
            std::printf("unknown line kind: %s\n", kind.c_str());
            return 1;
        }
    }
    return 0;
}
