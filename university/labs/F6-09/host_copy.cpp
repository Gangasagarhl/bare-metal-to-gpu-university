// F6-09 Listing 3: the host side of a pageable copy. A pageable transfer is first copied
// into a staging buffer by the CPU (CUDA 12.0 header, chapter source H1); this measures
// how fast this machine's CPU copies memory, with the harness protocol (3 + 21 runs).
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../F6-06/bench.h"

int main()
{
    const std::size_t bytes = std::size_t{64} << 20;     // 64 MiB
    std::vector<unsigned char> src(bytes, 1), dst(bytes, 0);
    auto once = [&]() {
        const auto t0 = std::chrono::steady_clock::now();
        std::memcpy(dst.data(), src.data(), bytes);
        const auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0).count();
    };
    bench::Row row{"cpu", "memcpy 64MiB", static_cast<long long>(bytes), 2.0 * bytes,
                   bench::summarize(bench::measure(once, 3, 21))};
    bench::printTable({row});
    std::printf("dst[last] = %d\n", dst[bytes - 1]);
    return 0;
}
