// zero_copy.cpp - DS401 F5-38, Listing 2: what a copy costs, and what "pinning" means here.
// Part 1: "send" a 1 MiB message 500 times, (a) by copying it into a separate send buffer
//         each time (what a socket write does inside the kernel), and (b) by handing over
//         only a descriptor (address, length) of a buffer that was prepared once.
// Part 2: the memory-locking limit of this process and an mlock() of a 1 MiB buffer:
//         registering memory for RDMA pins its pages in a similar way (see the chapter).
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#include <vector>

namespace {

struct Descriptor          // what a zero-copy interface passes instead of the bytes
{
    const char* addr;
    size_t length;
};

volatile uint64_t g_sink = 0;   // keeps the compiler from removing the work

void consume(const Descriptor& d)   // the "NIC" reads one byte per 4 KiB page of the message
{
    uint64_t s = 0;
    for (size_t i = 0; i < d.length; i += 4096) {
        s += static_cast<unsigned char>(d.addr[i]);
    }
    g_sink = g_sink + s;
}

}  // namespace

int main()
{
    constexpr size_t kBytes = 1 << 20;
    constexpr int kRounds = 500;
    std::vector<char> message(kBytes, 'm');
    std::vector<char> sendBuffer(kBytes);

    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kRounds; ++i) {
        std::memcpy(sendBuffer.data(), message.data(), kBytes);    // copy, then hand over
        consume(Descriptor{sendBuffer.data(), kBytes});
    }
    const auto t1 = std::chrono::steady_clock::now();
    for (int i = 0; i < kRounds; ++i) {
        consume(Descriptor{message.data(), kBytes});                // hand over in place
    }
    const auto t2 = std::chrono::steady_clock::now();
    const double copySec = std::chrono::duration<double>(t1 - t0).count();
    const double zeroSec = std::chrono::duration<double>(t2 - t1).count();
    const double gb = static_cast<double>(kBytes) * kRounds / 1e9;
    std::printf("part 1: %d messages of %zu bytes (%.2f GB in total)\n", kRounds, kBytes, gb);
    std::printf("  with a copy:       %8.2f ms  (%.2f GB/s of copying)\n", copySec * 1e3, gb / copySec);
    std::printf("  descriptor only:   %8.2f ms\n", zeroSec * 1e3);

    rlimit lim{};
    ::getrlimit(RLIMIT_MEMLOCK, &lim);
    const long page = ::sysconf(_SC_PAGESIZE);
    std::printf("part 2: page size %ld bytes; a %zu-byte buffer spans %zu pages\n", page, kBytes,
                kBytes / static_cast<size_t>(page));
    const auto show = [](rlim_t v) {
        if (v == RLIM_INFINITY) {
            std::printf("unlimited");
        } else {
            std::printf("%llu bytes", static_cast<unsigned long long>(v));
        }
    };
    std::printf("  RLIMIT_MEMLOCK soft limit: ");
    show(lim.rlim_cur);
    std::printf(", hard limit: ");
    show(lim.rlim_max);
    std::printf("\n");
    if (::mlock(message.data(), kBytes) == 0) {
        std::printf("  mlock(1 MiB): ok, the pages stay in RAM until munlock\n");
        ::munlock(message.data(), kBytes);
    } else {
        std::printf("  mlock(1 MiB) failed: %s\n", std::strerror(errno));
    }
    std::printf("  effective user id %d\n", static_cast<int>(::geteuid()));
    return 0;
}
