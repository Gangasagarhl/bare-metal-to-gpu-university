// F1-59 Listing 2: measure the copy bandwidth of THIS computer's host memory.
// It is a measurement of the build container's CPU memory, not of any GPU.
// Bytes counted: each copy reads the source and writes the destination (2 x size).
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

int main()
{
    const std::size_t size = std::size_t{64} * 1024 * 1024;     // 64 MiB per buffer
    std::vector<unsigned char> src(size, 1), dst(size, 0);
    std::vector<double> gbps;
    for (int rep = 0; rep < 11; ++rep) {
        auto t0 = std::chrono::steady_clock::now();
        std::memcpy(dst.data(), src.data(), size);
        auto t1 = std::chrono::steady_clock::now();
        double s = std::chrono::duration<double>(t1 - t0).count();
        gbps.push_back(2.0 * static_cast<double>(size) / s / 1e9);
        src[static_cast<std::size_t>(rep)] = static_cast<unsigned char>(rep);   // keep each copy necessary
    }
    std::sort(gbps.begin(), gbps.end());
    std::printf("buffer: %zu MiB, 11 copies, bytes counted per copy: read + write\n", size / (1024 * 1024));
    std::printf("slowest %.1f GB/s, median %.1f GB/s, fastest %.1f GB/s (decimal GB = 1e9 bytes)\n",
                gbps.front(), gbps[gbps.size() / 2], gbps.back());
    std::printf("check byte: %d\n", static_cast<int>(dst[5]));
    return 0;
}
