// u_badptr.cc - hand the kernel's write call pointers it must not trust (B12 test).
#include "ulib.h"

namespace {
int failures = 0;

void check(const char* what, int64_t got, int64_t want)
{
    print("badptr: ");
    print(what);
    print(" -> ");
    print_num(got);
    print(got == want ? " (as expected)\n" : " (WRONG)\n");
    failures += got != want;
}
}  // namespace

int main()
{
    char top[8] = {'x'};
    check("valid buffer, 3 bytes      ", sys_write(1, "ok\n", 3), 3);
    check("unmapped user address      ", sys_write(1, reinterpret_cast<void*>(0x9000000000), 16), -2);
    check("kernel address 0x100000    ", sys_write(1, reinterpret_cast<void*>(0x100000), 16), -2);
    // Starts in our stack page, runs 8 KiB beyond: the copy faults half way.
    check("length past mapped memory  ", sys_write(1, top, 8192), -2);
    check("length that wraps past 2^64", sys_write(1, top, ~0ull - 15), -2);
    check("bad file descriptor        ", sys_write(7, "x", 1), -3);
    return failures;
}
