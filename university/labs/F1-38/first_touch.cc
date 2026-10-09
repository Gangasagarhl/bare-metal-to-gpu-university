// "The first run is slow": fill a fresh 256 MiB buffer three times and time each pass. The kernel's
// count of minor page faults for this process (getrusage) is read before and after each pass.
// Times are measurements on the machine that ran this program, not specifications.
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <sys/resource.h>

long minorFaults()
{
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_minflt;
}

int main()
{
    const std::size_t bytes = 256u * 1024 * 1024;
    auto buffer = std::make_unique_for_overwrite<unsigned char[]>(bytes);  // not touched yet
    for (int pass = 1; pass <= 3; ++pass) {
        const long faultsBefore = minorFaults();
        const auto t0 = std::chrono::steady_clock::now();
        std::fill(buffer.get(), buffer.get() + bytes, static_cast<unsigned char>(pass));
        const auto t1 = std::chrono::steady_clock::now();
        const long faults = minorFaults() - faultsBefore;
        std::printf("pass %d: %7.1f ms, minor page faults during the pass: %ld\n", pass,
                    std::chrono::duration<double, std::milli>(t1 - t0).count(), faults);
    }
    std::printf("buffer size %zu bytes = %zu pages of 4096 bytes\n", bytes, bytes / 4096);
    std::printf("(checksum %u)\n", unsigned{buffer[bytes / 2]});
    return 0;
}
