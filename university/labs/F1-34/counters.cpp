// Can this machine count hardware events for us? Ask the Linux kernel for three counters
// with the perf_event_open system call and print what it answers.
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>

void tryCounter(const char* name, std::uint32_t type, std::uint64_t config)
{
    perf_event_attr attr;
    std::memset(&attr, 0, sizeof attr);
    attr.size = sizeof attr;
    attr.type = type;
    attr.config = config;
    attr.disabled = 1;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    errno = 0;
    const long fd = syscall(SYS_perf_event_open, &attr, 0, -1, -1, 0);  // this process, any CPU
    if (fd < 0) {
        std::printf("%-28s not available: %s\n", name, std::strerror(errno));
    } else {
        std::printf("%-28s available\n", name);
        close(static_cast<int>(fd));
    }
}

int main()
{
    tryCounter("hardware: cache misses", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    tryCounter("hardware: instructions", PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
    tryCounter("software: page faults", PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS);
    return 0;
}
