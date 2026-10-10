// ustart.h - BR-06: the only CPU-specific part of diskcheck.cc: the entry point and three
// Linux system calls (read, write, exit) for four CPUs, so that one freestanding program
// runs under Linux on x86-64 and, through QEMU user-mode emulation, on AArch64, RISC-V
// and the big-endian s390x. System-call numbers and registers are recalled from memory
// (the same ones F4-30's lab uses); the run is the evidence: a wrong number prints nothing.
#pragma once

extern "C" [[noreturn]] void umain();

namespace sys {

inline long call3(long nr, long a0, long a1, long a2)
{
#if defined(__x86_64__)
    long ret;
    asm volatile("syscall" : "=a"(ret) : "a"(nr), "D"(a0), "S"(a1), "d"(a2) : "rcx", "r11", "memory");
    return ret;
#elif defined(__aarch64__)
    register long x8 asm("x8") = nr, x0 asm("x0") = a0, x1 asm("x1") = a1, x2 asm("x2") = a2;
    asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
    return x0;
#elif defined(__riscv)
    register long a7 asm("a7") = nr, r0 asm("a0") = a0, r1 asm("a1") = a1, r2 asm("a2") = a2;
    asm volatile("ecall" : "+r"(r0) : "r"(a7), "r"(r1), "r"(r2) : "memory");
    return r0;
#elif defined(__s390x__)
    register long r1 asm("r1") = nr, r2 asm("r2") = a0, r3 asm("r3") = a1, r4 asm("r4") = a2;
    asm volatile("svc 0" : "+r"(r2) : "r"(r1), "r"(r3), "r"(r4) : "memory");
    return r2;
#else
#error "no system-call stub for this CPU"
#endif
}

#if defined(__x86_64__)
constexpr long kRead = 0, kWrite = 1, kExit = 60;
#elif defined(__aarch64__) || defined(__riscv)
constexpr long kRead = 63, kWrite = 64, kExit = 93;
#else
constexpr long kRead = 3, kWrite = 4, kExit = 1;   // s390x
#endif

inline long read(int fd, void* buf, long n) { return call3(kRead, fd, reinterpret_cast<long>(buf), n); }
inline long write(int fd, const void* buf, long n) { return call3(kWrite, fd, reinterpret_cast<long>(buf), n); }
[[noreturn]] inline void exit(int code)
{
    call3(kExit, code, 0, 0);
    for (;;) {
    }
}

} // namespace sys

// _start: the kernel gives us a stack and nothing else; keep it aligned and call umain.
#if defined(__x86_64__)
asm(".text\n.globl _start\n_start:\n xor %ebp, %ebp\n and $-16, %rsp\n call umain\n hlt\n");
#elif defined(__aarch64__)
asm(".text\n.globl _start\n_start:\n mov x29, #0\n bl umain\n brk #0\n");
#elif defined(__riscv)
asm(".text\n.globl _start\n_start:\n li ra, 0\n call umain\n unimp\n");
#elif defined(__s390x__)
asm(".text\n.globl _start\n_start:\n aghi %r15, -160\n brasl %r14, umain\n .long 0\n");
#endif

// The compiler may call memcpy for struct copies even in freestanding code.
extern "C" void* memcpy(void* d, const void* s, decltype(sizeof 0) n)
{
    auto* dp = static_cast<unsigned char*>(d);
    const auto* sp = static_cast<const unsigned char*>(s);
    for (decltype(n) i = 0; i < n; ++i) {
        dp[i] = sp[i];
    }
    return d;
}
