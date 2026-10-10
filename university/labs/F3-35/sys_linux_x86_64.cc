// sys_linux_x86_64.cc - F3-35: tlibc's system-call layer for Linux on x86-64.
// Convention (System V AMD64 psABI, Linux kernel interface; checked by the lab run): number in rax,
// arguments in rdi, rsi, rdx; the syscall instruction; result in rax; rcx and r11 are clobbered.
#include <stddef.h>

asm(R"(
    .text
    .globl _start
_start:                         # the kernel jumps here with argc at (%rsp)
    xor %rbp, %rbp              # mark the outermost stack frame
    mov %rsp, %rdi              # argument 1 of tlibc_start: the initial stack pointer
    and $-16, %rsp              # the ABI wants a 16-byte aligned stack at a call
    call tlibc_start
    hlt
)");

namespace {
constexpr long SYS_write = 1, SYS_exit_group = 231;   // Linux x86-64 system-call numbers

long syscall3(long nr, long a, long b, long c)
{
    long ret;
    asm volatile("syscall" : "=a"(ret) : "a"(nr), "D"(a), "S"(b), "d"(c) : "rcx", "r11", "memory");
    return ret;
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
