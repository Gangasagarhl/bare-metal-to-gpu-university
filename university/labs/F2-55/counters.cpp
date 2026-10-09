// counters.cpp - ask the Linux kernel for performance counters with perf_event_open,
// the system call that the perf tool is built on, and report honestly what we get.
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>

// glibc has no wrapper for this system call, so it is called through syscall().
int openCounter(std::uint32_t type, std::uint64_t config)
{
    perf_event_attr attr;
    std::memset(&attr, 0, sizeof attr);
    attr.type = type;
    attr.size = sizeof attr;
    attr.config = config;
    attr.disabled = 1;        // start stopped; we enable it around the measured code
    attr.exclude_kernel = 1;  // count only our own user-space work
    attr.exclude_hv = 1;
    return static_cast<int>(syscall(SYS_perf_event_open, &attr, 0, -1, -1, 0));
}

double busyWork()
{
    double x = 1.0;
    for (int i = 0; i < 10'000'000; ++i) {
        x = x * 1.0000001 + 1e-9;
    }
    return x;
}

void measure(char const* name, std::uint32_t type, std::uint64_t config)
{
    int const fd = openCounter(type, config);
    if (fd < 0) {
        int const e = errno;
        std::printf("%-26s not available: perf_event_open failed, errno %d (%s)\n", name, e,
                    std::strerror(e));
        return;
    }
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    double const r = busyWork();
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    std::uint64_t value = 0;
    ssize_t const got = read(fd, &value, sizeof value);
    close(fd);
    if (got != static_cast<ssize_t>(sizeof value)) {
        std::printf("%-26s read failed\n", name);
        return;
    }
    std::printf("%-26s %llu  (work result %.3f)\n", name, static_cast<unsigned long long>(value), r);
}

int main()
{
    measure("hw: cpu-cycles", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
    measure("hw: instructions", PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
    measure("hw: branch-misses", PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES);
    measure("hw: cache-misses", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    measure("sw: task-clock (ns)", PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK);
    measure("sw: page-faults", PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS);
    return 0;
}
