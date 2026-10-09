// nolibc_hello.cpp - F3-50: a program with no C library at all, for three architectures.
// Everything a C library would do is here in a few lines: the entry point, the system-call
// instruction and its register convention, and the exit. Built freestanding and static.
#include <asm/unistd.h>   // the kernel's own list of system-call numbers for the target

#if defined(__x86_64__)
#define ARCH_NAME "x86-64"
asm(".globl _start\n_start:\n"
    "  xor %rbp, %rbp\n"          // outermost frame
    "  mov %rsp, %rdi\n"          // argument 1: the initial stack pointer (argc is at sp[0])
    "  and $-16, %rsp\n"          // the psABI wants 16-byte alignment at a call
    "  call cmain\n"
    "  hlt\n");
static long sys3(long nr, long a, long b, long c)
{
    long ret;
    asm volatile("syscall" : "=a"(ret) : "a"(nr), "D"(a), "S"(b), "d"(c) : "rcx", "r11", "memory");
    return ret;
}
#elif defined(__aarch64__)
#define ARCH_NAME "AArch64"
asm(".globl _start\n_start:\n"
    "  mov x29, #0\n"
    "  mov x0, sp\n"
    "  bl cmain\n"
    "  b .\n");
static long sys3(long nr, long a, long b, long c)
{
    register long x8 asm("x8") = nr;
    register long x0 asm("x0") = a;
    register long x1 asm("x1") = b;
    register long x2 asm("x2") = c;
    asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
    return x0;
}
#elif defined(__riscv) && __riscv_xlen == 64
#define ARCH_NAME "RISC-V 64"
asm(".globl _start\n_start:\n"
    "  .option push\n  .option norelax\n"
    "  la gp, __global_pointer$\n"   // the linker may use gp-relative addressing
    "  .option pop\n"
    "  mv a0, sp\n"
    "  call cmain\n"
    "  j .\n");
static long sys3(long nr, long a, long b, long c)
{
    register long a7 asm("a7") = nr;
    register long a0 asm("a0") = a;
    register long a1 asm("a1") = b;
    register long a2 asm("a2") = c;
    asm volatile("ecall" : "+r"(a0) : "r"(a7), "r"(a1), "r"(a2) : "memory");
    return a0;
}
#else
#error "unsupported architecture"
#endif

namespace {

long length(const char* s)
{
    long n = 0;
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

void put(const char* s)
{
    sys3(__NR_write, 1, reinterpret_cast<long>(s), length(s));
}

void put_number(long v)
{
    char buf[24];
    int i = 23;
    buf[i] = '\0';
    do {
        buf[--i] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v != 0 && i > 0);
    put(buf + i);
}

} // namespace

extern "C" [[noreturn]] void cmain(long* sp)
{
    put("hello from " ARCH_NAME " with no C library\n");
    put("write is system call ");
    put_number(__NR_write);
    put(", exit is ");
    put_number(__NR_exit);
    put("; argc = ");
    put_number(sp[0]);
    put("\n");
    for (;;) {
        sys3(__NR_exit, 0, 0, 0);
    }
}
