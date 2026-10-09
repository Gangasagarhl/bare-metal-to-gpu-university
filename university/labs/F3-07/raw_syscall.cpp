// raw_syscall.cpp - three ways to reach the same system call, and two calls the kernel refuses.
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/syscall.h>
#include <unistd.h>

int main()
{
    std::printf("1. printf (C library, buffered)\n");
    std::fflush(stdout);  // push the buffer out now
    const char a[] = "2. write() (thin C library wrapper)\n";
    if (write(1, a, sizeof a - 1) < 0) { return 1; }
    const char b[] = "3. syscall(SYS_write, ...) (number chosen by hand)\n";
    if (syscall(SYS_write, 1, b, sizeof b - 1) < 0) { return 1; }

    std::printf("SYS_write is number %d, SYS_getpid is number %d on this machine\n", SYS_write,
                SYS_getpid);
    std::printf("getpid() equals syscall(SYS_getpid): %s\n",
                getpid() == syscall(SYS_getpid) ? "yes" : "no");

    // A pointer the process does not own: the kernel checks it and refuses.
    volatile std::uintptr_t badAddress = 1;  // volatile: the compiler cannot see the value
    const char* bad = reinterpret_cast<const char*>(badAddress);
    errno = 0;
    const long r1 = write(1, bad, 5);
    std::printf("write with a bad pointer returned %ld, errno %d (%s)\n", r1, errno,
                std::strerror(errno));

    // A system-call number the kernel does not have.
    errno = 0;
    const long r2 = syscall(99999);
    std::printf("system call 99999 returned %ld, errno %d (%s)\n", r2, errno, std::strerror(errno));
    return 0;
}
