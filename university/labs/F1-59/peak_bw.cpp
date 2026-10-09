// F1-59 Listing 1: the peak bandwidth of a memory interface, from its datasheet terms,
// and the shortest possible time for a memory-bound kernel (SAXPY) on it.
// Input lines: label busWidthBits dataRateGTps elements
//   busWidthBits  width of the whole memory interface in bits
//   dataRateGTps  transfers per second on each data pin, in billions (GT/s)
//   elements      SAXPY length n; SAXPY moves 12 bytes per element (read x, read y, write y)
#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    std::string label;
    double widthBits = 0.0, rateGT = 0.0, n = 0.0;
    std::printf("%-26s %7s %7s | %10s %10s | %12s %12s\n", "case", "bits", "GT/s", "peak GB/s", "peak GiB/s",
                "SAXPY MB", "min time us");
    while (std::cin >> label >> widthBits >> rateGT >> n) {
        const double bytesPerTransfer = widthBits / 8.0;               // bits -> bytes
        const double peak = bytesPerTransfer * rateGT * 1e9;           // bytes per second
        const double moved = 12.0 * n;                                 // bytes SAXPY must move
        const double minTime = moved / peak;                           // seconds, at 100 % of peak
        std::printf("%-26s %7.0f %7.2f | %10.1f %10.1f | %12.3f %12.3f\n", label.c_str(), widthBits, rateGT,
                    peak / 1e9, peak / (1024.0 * 1024.0 * 1024.0), moved / 1e6, minTime * 1e6);
    }
    return 0;
}
