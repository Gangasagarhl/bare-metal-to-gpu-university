// compat.cpp - F3-50: a model, inside one ordinary program, of the two system-call ABI paths.
// Path 1 ("own ABI"): our numbers; every call returns a pair {value, error}.
// Path 2 ("Linux-compatible layer"): Linux numbers in; one long out; an error is returned
// as minus the error number. A C-library wrapper turns that into -1 plus errno.
// Nothing here enters a real kernel: the "kernel" is a few functions with a console string.
#include <array>
#include <cerrno>
#include <cstdio>
#include <string>

namespace {

struct OwnResult
{
    long value;
    int error;                           // 0 means success
};

std::string console;                     // what the model kernel's console has shown

// ---- the model kernel, with its own ABI -------------------------------------------------
OwnResult k_getpid(long, long, long) { return {42, 0}; }

OwnResult k_write(long fd, long buf, long n)
{
    if (fd != 1) {
        return {0, EBADF};               // only descriptor 1 (the console) is open
    }
    console.append(reinterpret_cast<const char*>(buf), static_cast<std::size_t>(n));
    return {n, 0};
}

using Handler = OwnResult (*)(long, long, long);
enum OwnNumber { OWN_GETPID = 0, OWN_WRITE = 1, OWN_COUNT = 2 };
constexpr std::array<Handler, OWN_COUNT> own_table = {k_getpid, k_write};

OwnResult own_syscall(long nr, long a, long b, long c)
{
    if (nr < 0 || nr >= OWN_COUNT) {
        return {0, ENOSYS};
    }
    OwnResult r = own_table[static_cast<std::size_t>(nr)](a, b, c);
    std::printf("      [kernel] own call %ld -> {value %ld, error %d}\n", nr, r.value, r.error);
    return r;
}

// ---- path 2: the Linux-compatible layer (numbers as on Linux x86-64) -----------------------
constexpr long LINUX_WRITE = 1, LINUX_GETPID = 39;

long linux_syscall(long nr, long a, long b, long c)
{
    long own = -1;
    if (nr == LINUX_WRITE) own = OWN_WRITE;
    if (nr == LINUX_GETPID) own = OWN_GETPID;
    if (own < 0) {
        return -ENOSYS;
    }
    OwnResult r = own_syscall(own, a, b, c);
    return r.error != 0 ? -static_cast<long>(r.error) : r.value;
}

// ---- a C-library wrapper on top of path 2 --------------------------------------------------
long libc_write(int fd, const char* buf, long n)
{
    long r = linux_syscall(LINUX_WRITE, fd, reinterpret_cast<long>(buf), n);
    if (r < 0) {
        errno = static_cast<int>(-r);
        return -1;
    }
    return r;
}

} // namespace

int main()
{
    const char hi[] = "hi\n";
    std::printf("path 1, own ABI:\n");
    OwnResult a = own_syscall(OWN_WRITE, 1, reinterpret_cast<long>(hi), 3);
    std::printf("   write(1)  -> value %ld, error %d\n", a.value, a.error);
    OwnResult b = own_syscall(OWN_WRITE, 7, reinterpret_cast<long>(hi), 3);
    std::printf("   write(7)  -> value %ld, error %d\n", b.value, b.error);
    OwnResult c = own_syscall(9, 0, 0, 0);
    std::printf("   call 9    -> value %ld, error %d\n", c.value, c.error);

    std::printf("path 2, Linux-compatible layer:\n");
    std::printf("   getpid    -> %ld\n", linux_syscall(LINUX_GETPID, 0, 0, 0));
    std::printf("   write(7)  -> %ld\n", linux_syscall(LINUX_WRITE, 7, reinterpret_cast<long>(hi), 3));
    std::printf("   call 500  -> %ld\n", linux_syscall(500, 0, 0, 0));

    std::printf("C-library wrapper over path 2:\n");
    errno = 0;
    long w = libc_write(1, hi, 3);
    std::printf("   write(1)  -> %ld, errno %d\n", w, errno);
    errno = 0;
    w = libc_write(7, hi, 3);
    std::printf("   write(7)  -> %ld, errno %d\n", w, errno);
    std::printf("console of the model kernel: \"%s\" (%zu bytes)\n",
                console == "hi\nhi\n" ? "hi\\nhi\\n" : console.c_str(), console.size());
    return 0;
}
