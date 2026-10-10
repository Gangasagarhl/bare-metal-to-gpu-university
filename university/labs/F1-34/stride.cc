// Stride benchmark ("memory mountain"): read throughput for working sets of growing size and
// growing stride. Stride 1 reads every 8-byte element; stride 8 reads one element per 64 bytes.
// GB/s counts only the bytes the program asked for. Measurements, not specifications.
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

double gbPerSecond(const std::vector<std::uint64_t>& data, std::size_t stride, std::uint64_t& sink)
{
    const std::size_t perPass = data.size() / stride;              // elements read per pass
    const std::size_t passes = (std::size_t{1} << 25) / perPass + 1; // about 32 million reads
    const auto t0 = std::chrono::steady_clock::now();
    for (std::size_t p = 0; p < passes; ++p) {
        std::uint64_t sum = 0;
        for (std::size_t i = 0; i < data.size(); i += stride) {
            sum += data[i];
        }
        sink += sum;
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double bytes = double(passes) * double(perPass) * sizeof(std::uint64_t);
    return bytes / std::chrono::duration<double, std::nano>(t1 - t0).count();
}

int main()
{
    const std::size_t strides[] = {1, 2, 4, 8, 16, 32};
    std::uint64_t sink = 0;
    std::printf("read throughput in GB/s\n");
    std::printf("(rows: working set; columns: stride in 8-byte elements)\n");
    std::printf("%12s", "size \\ stride");
    for (const std::size_t s : strides) {
        std::printf(" %7zu", s);
    }
    std::printf("\n");
    for (std::size_t bytes = 16 * 1024; bytes <= 256u * 1024 * 1024; bytes *= 4) {
        const std::vector<std::uint64_t> data(bytes / sizeof(std::uint64_t), 1);
        std::printf("%9zu KiB", bytes / 1024);
        for (const std::size_t s : strides) {
            std::printf(" %7.2f", gbPerSecond(data, s, sink));
        }
        std::printf("\n");
    }
    std::printf("(checksum %llu)\n", static_cast<unsigned long long>(sink));
    return 0;
}
