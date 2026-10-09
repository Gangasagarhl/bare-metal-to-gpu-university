// forensic_logger.cpp - F3-50 forensic evidence generator. A port's logger, built on the
// project's Linux-compatible layer, writes three records to its log descriptor and checks
// the return value of every write, as good code should. The layer and the C-library wrapper
// below are copied from the port's branch "compat-v2" as they were on the day of the report.
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

struct OwnResult
{
    long value;
    int error;
};

std::string console;                         // the model kernel's console (descriptor 1)

OwnResult k_write(long fd, long buf, long n)
{
    if (fd != 1) {
        return {0, EBADF};
    }
    console.append(reinterpret_cast<const char*>(buf), static_cast<std::size_t>(n));
    return {n, 0};
}

OwnResult own_syscall_traced(long a, long b, long c)
{
    OwnResult r = k_write(a, b, c);
    std::printf("trace: own write(fd %ld, %ld bytes) = {value %ld, error %d}\n", a, c, r.value, r.error);
    return r;
}

long linux_syscall_write(long fd, long buf, long n)        // compat-v2
{
    OwnResult r = own_syscall_traced(fd, buf, n);
    return r.error != 0 ? r.error : r.value;
}

long libc_write(int fd, const char* buf, long n)
{
    long r = linux_syscall_write(fd, reinterpret_cast<long>(buf), n);
    if (r < 0) {
        errno = static_cast<int>(-r);
        return -1;
    }
    return r;
}

} // namespace

int main()
{
    const int log_fd = 5;                    // the logger believes its log file is descriptor 5
    const char* records[] = {"boot ok\n", "mounted /data\n", "service started\n"};
    for (const char* rec : records) {
        long n = static_cast<long>(std::strlen(rec));
        errno = 0;
        long w = libc_write(log_fd, rec, n);
        if (w < 0) {
            std::printf("logger: write failed: %s\n", std::strerror(errno));
        } else if (w != n) {
            std::printf("logger: short write, %ld of %ld bytes; retrying the rest later\n", w, n);
        } else {
            std::printf("logger: record written (%ld bytes)\n", w);
        }
    }
    std::printf("bytes that reached any open file or the console: %zu\n", console.size());
    return 0;
}
