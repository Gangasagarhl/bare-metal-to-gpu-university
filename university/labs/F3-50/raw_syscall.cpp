// raw_syscall.cpp - F3-50: the same system calls made three ways on Linux x86-64:
// through the C library's wrapper, through syscall() and through the bare instruction.
// The point: the kernel and the C library use two different error conventions.
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace {

// The bare instruction (x86-64 Linux convention, checked by this run): number in rax,
// arguments in rdi, rsi, rdx; result in rax; the CPU overwrites rcx and r11.
long bare3(long nr, long a, long b, long c)
{
    long ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(nr), "D"(a), "S"(b), "d"(c)
                 : "rcx", "r11", "memory");
    return ret;
}

// A fourth argument: Linux x86-64 passes it in r10 (rcx is taken by the instruction itself).
long bare4(long nr, long a, long b, long c, long d)
{
    long ret;
    register long r10 asm("r10") = d;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(nr), "D"(a), "S"(b), "d"(c), "r"(r10)
                 : "rcx", "r11", "memory");
    return ret;
}

void libc_result(const char* how, long ret)
{
    int e = errno;
    std::printf("   %-18s: returned %ld", how, ret);
    if (ret == -1) {
        std::printf(", errno %d (%s)", e, std::strerror(e));
    }
    std::printf("\n");
}

} // namespace

int main()
{
    std::printf("system-call numbers from <sys/syscall.h>: write %d, getpid %d\n",
                SYS_write, SYS_getpid);

    const char msg[] = "hello from the bare syscall instruction\n";
    std::printf("1. write(1, msg, %zu) with the bare instruction:\n", sizeof msg - 1);
    std::fflush(stdout);                       // keep stdio's buffer and our raw write in order
    long r = bare3(SYS_write, 1, reinterpret_cast<long>(msg), sizeof msg - 1);
    std::printf("   bare instruction  : returned %ld\n", r);

    std::printf("2. write to descriptor 99, which is not open:\n");
    r = bare3(SYS_write, 99, reinterpret_cast<long>(msg), 1);
    std::printf("   bare instruction  : returned %ld (minus EBADF; EBADF is %d)\n", r, EBADF);
    errno = 0;
    libc_result("libc write()", write(99, msg, 1));

    std::printf("3. a system-call number the kernel does not have (1000):\n");
    r = bare3(1000, 0, 0, 0);
    std::printf("   bare instruction  : returned %ld (minus ENOSYS; ENOSYS is %d)\n", r, ENOSYS);
    errno = 0;
    libc_result("libc syscall(1000)", syscall(1000));

    long p1 = getpid();
    long p2 = syscall(SYS_getpid);
    long p3 = bare3(SYS_getpid, 0, 0, 0);
    std::printf("4. getpid three ways gives the same process id: %s\n",
                (p1 == p2 && p2 == p3) ? "yes" : "no");

    char path[] = "/tmp/os402-r10-test";
    unlink(path);
    mode_t old_mask = umask(0);                // so the file gets exactly the requested mode
    long fd = bare4(SYS_openat, AT_FDCWD, reinterpret_cast<long>(path), O_CREAT | O_EXCL | O_WRONLY, 0640);
    struct stat st {};
    bool mode_ok = fd >= 0 && fstat(static_cast<int>(fd), &st) == 0 && (st.st_mode & 0777) == 0640;
    umask(old_mask);
    std::printf("5. openat with mode 0640 as the fourth argument (in r10): file mode %03o, %s\n",
                static_cast<unsigned>(st.st_mode & 0777), mode_ok ? "as requested" : "WRONG");
    if (fd >= 0) close(static_cast<int>(fd));
    unlink(path);
    return 0;
}
