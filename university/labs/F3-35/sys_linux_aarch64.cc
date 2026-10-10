// sys_linux_aarch64.cc - F3-35: tlibc's system-call layer for Linux on AArch64.
// Convention (Linux kernel interface for arm64; checked by the lab run under qemu-aarch64):
// number in x8, arguments in x0..x2, the "svc #0" instruction, result in x0.
#include <stddef.h>

asm(R"(
    .text
    .globl _start
_start:                         // the kernel jumps here with argc at [sp]
    mov x29, #0                 // mark the outermost stack frame
    mov x0, sp                  // argument 1 of tlibc_start: the initial stack pointer
    bl tlibc_start
    brk #0
)");

namespace {
constexpr long SYS_write = 64, SYS_exit_group = 94;   // Linux arm64 system-call numbers

long syscall3(long nr, long a, long b, long c)
{
    register long x8 asm("x8") = nr;
    register long x0 asm("x0") = a;
    register long x1 asm("x1") = b;
    register long x2 asm("x2") = c;
    asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
    return x0;
}
} // namespace

extern "C" long tlibc_sys_write(int fd, const void* buf, size_t n)
{
    return syscall3(SYS_write, fd, reinterpret_cast<long>(buf), static_cast<long>(n));
}

extern "C" [[noreturn]] void tlibc_sys_exit(int status)
{
    syscall3(SYS_exit_group, status, 0, 0);
    __builtin_unreachable();
}
