// F6-31 Listing 2: demand paging on the CPU, measured for real in this build.
// The operating system gives a fresh mapping physical pages only when a page is
// first touched (a minor page fault each time). Managed memory on a GPU uses the
// same idea between two processors. This program counts the faults of
//   pass 1  first touch of a fresh 256 MiB mapping
//   pass 2  the same loop again (pages already present)
//   pass 3  a second mapping asked to be populated in advance (MAP_POPULATE)
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#include <chrono>
#include <cstdio>
#include <cstring>

long minorFaults()
{
    rusage u{};
    getrusage(RUSAGE_SELF, &u);
    return u.ru_minflt;
}

// Writes one byte per 4096 bytes and reports faults and time for that loop only.
void touch(const char* label, unsigned char* p, std::size_t bytes)
{
    const long before = minorFaults();
    const auto t0 = std::chrono::steady_clock::now();
    for (std::size_t off = 0; off < bytes; off += 4096) {
        p[off] = static_cast<unsigned char>(off >> 12);
    }
    const auto t1 = std::chrono::steady_clock::now();
    const long faults = minorFaults() - before;
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::printf("%-26s minor faults %8ld   time %8.3f ms\n", label, faults, ms);
}

int main()
{
    const std::size_t bytes = std::size_t{256} << 20;
    std::printf("page size reported by the OS: %ld bytes\n", sysconf(_SC_PAGESIZE));
    std::printf("mapping size: %zu bytes = %zu pages of 4096 bytes\n", bytes, bytes / 4096);
    void* a = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (a == MAP_FAILED) {
        std::perror("mmap");
        return 1;
    }
    madvise(a, bytes, MADV_NOHUGEPAGE);                   // keep 4 KiB pages for a clear count
    touch("pass 1: first touch", static_cast<unsigned char*>(a), bytes);
    touch("pass 2: touch again", static_cast<unsigned char*>(a), bytes);
    const long beforePopulate = minorFaults();
    const auto t0 = std::chrono::steady_clock::now();
    void* b = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    const auto t1 = std::chrono::steady_clock::now();
    if (b == MAP_FAILED) {
        std::perror("mmap");
        return 1;
    }
    std::printf("%-26s minor faults %8ld   time %8.3f ms\n", "populate in advance",
                minorFaults() - beforePopulate,
                std::chrono::duration<double, std::milli>(t1 - t0).count());
    touch("pass 3: touch populated", static_cast<unsigned char*>(b), bytes);
    munmap(a, bytes);
    munmap(b, bytes);
    return 0;
}
