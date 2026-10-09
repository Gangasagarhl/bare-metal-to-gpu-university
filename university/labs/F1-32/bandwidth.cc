// Read bandwidth: sum a block of 64-bit numbers again and again, for blocks of growing size.
// The loads are independent of each other, so the hardware may have many in flight at once.
// Numbers are measurements on the machine that ran this program, not specifications.
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

int main()
{
    const std::size_t totalBytes = 4ull * 1024 * 1024 * 1024;  // read 4 GiB in total per size
    std::printf("%14s %12s\n", "block size", "GB/s read");
    std::uint64_t sink = 0;
    for (std::size_t bytes = 16 * 1024; bytes <= 512u * 1024 * 1024; bytes *= 4) {
        std::vector<std::uint64_t> data(bytes / sizeof(std::uint64_t), 1);
        const std::size_t repeats = totalBytes / bytes;
        const auto t0 = std::chrono::steady_clock::now();
        for (std::size_t r = 0; r < repeats; ++r) {
            std::uint64_t sum = 0;
            for (const std::uint64_t x : data) {
                sum += x;
            }
            sink += sum;
        }
        const auto t1 = std::chrono::steady_clock::now();
        const double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
        std::printf("%10zu KiB %12.2f\n", bytes / 1024, double(repeats * bytes) / ns);
    }
    std::printf("(checksum %llu)\n", static_cast<unsigned long long>(sink));
    return 0;
}
