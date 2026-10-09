// Reading versus writing a large block (256 MiB): GB/s for a read-only sum, a write-only fill,
// and a copy (read one block, write another). Measurements, not specifications.
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

template <typename F>
double gbPerSecond(std::size_t bytesMoved, F work)
{
    work();                                                    // warm-up pass (first touch)
    const auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < 4; ++r) {
        work();
    }
    const auto t1 = std::chrono::steady_clock::now();
    return 4.0 * double(bytesMoved) / std::chrono::duration<double, std::nano>(t1 - t0).count();
}

int main()
{
    const std::size_t n = (256u * 1024 * 1024) / sizeof(std::uint64_t);
    std::vector<std::uint64_t> a(n, 1);
    std::vector<std::uint64_t> b(n, 2);
    std::uint64_t sink = 0;
    const std::size_t bytes = n * sizeof(std::uint64_t);
    const double readOnly = gbPerSecond(bytes, [&] {
        std::uint64_t s = 0;
        for (const std::uint64_t x : a) {
            s += x;
        }
        sink += s;
    });
    std::uint64_t value = 0;
    const double writeOnly = gbPerSecond(bytes, [&] { std::fill(b.begin(), b.end(), ++value); });
    const double copy = gbPerSecond(2 * bytes, [&] { std::copy(a.begin(), a.end(), b.begin()); });
    std::printf("read-only  sum : %6.2f GB/s (bytes the program read)\n", readOnly);
    std::printf("write-only fill: %6.2f GB/s (bytes the program wrote)\n", writeOnly);
    std::printf("copy a -> b    : %6.2f GB/s (bytes read + bytes written)\n", copy);
    std::printf("(checksum %llu)\n", static_cast<unsigned long long>(sink + b[n / 2]));
    return 0;
}
