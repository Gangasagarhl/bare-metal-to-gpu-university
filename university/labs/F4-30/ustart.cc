// ustart.cc - F4-30: everything CPU-specific in the user-mode test runner: the entry point
// (_start) and two Linux system calls, write and exit. System-call numbers, registers and
// instructions are recalled from memory (chapter F4-30, unverified box); the lab run is the
// evidence that each one works under qemu-user: if a number were wrong, nothing would print.
#include <cstddef>
#include <cstdint>
#include "arch.h"
#include "kprint.h"
#include "neutral_tests.h"
#include "serial.h"

extern "C" void run_global_constructors();
extern "C" [[noreturn]] void ustart_main();

namespace {

long sys2(long nr, long a0, long a1, long a2)
{
#if defined(__x86_64__)
    long ret;
    asm volatile("syscall" : "=a"(ret) : "a"(nr), "D"(a0), "S"(a1), "d"(a2) : "rcx", "r11", "memory");
    return ret;
#elif defined(__i386__)
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(nr), "b"(a0), "c"(a1), "d"(a2) : "memory");
    return ret;
#elif defined(__aarch64__)
    register long x8 asm("x8") = nr, x0 asm("x0") = a0, x1 asm("x1") = a1, x2 asm("x2") = a2;
    asm volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
    return x0;
#elif defined(__arm__)
    register long r7 asm("r7") = nr, r0 asm("r0") = a0, r1 asm("r1") = a1, r2 asm("r2") = a2;
    asm volatile("svc #0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2) : "memory");
    return r0;
#elif defined(__riscv)
    register long a7 asm("a7") = nr, r0 asm("a0") = a0, r1 asm("a1") = a1, r2 asm("a2") = a2;
    asm volatile("ecall" : "+r"(r0) : "r"(a7), "r"(r1), "r"(r2) : "memory");
    return r0;
#elif defined(__loongarch__)
    register long a7 asm("$a7") = nr, r0 asm("$a0") = a0, r1 asm("$a1") = a1, r2 asm("$a2") = a2;
    asm volatile("syscall 0" : "+r"(r0) : "r"(a7), "r"(r1), "r"(r2)
                 : "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "memory");
    return r0;
#elif defined(__mips__)
    register long v0 asm("$2") = nr, r0 asm("$4") = a0, r1 asm("$5") = a1, r2 asm("$6") = a2;
    register long a3 asm("$7");
    asm volatile("syscall" : "+r"(v0), "=r"(a3) : "r"(r0), "r"(r1), "r"(r2)
                 : "$1", "$3", "$8", "$9", "$10", "$11", "$12", "$13", "$14", "$15", "$24", "$25", "hi", "lo",
                   "memory");
    return a3 != 0 ? -v0 : v0;
#elif defined(__powerpc__)
    register long r0 asm("r0") = nr, r3 asm("r3") = a0, r4 asm("r4") = a1, r5 asm("r5") = a2;
    asm volatile("sc" : "+r"(r0), "+r"(r3), "+r"(r4), "+r"(r5)
                 : : "r6", "r7", "r8", "r9", "r10", "r11", "r12", "cr0", "ctr", "xer", "memory");
    return r3;
#elif defined(__s390x__)
    register long r1 asm("r1") = nr, r2 asm("r2") = a0, r3 asm("r3") = a1, r4 asm("r4") = a2;
    asm volatile("svc 0" : "+r"(r2) : "r"(r1), "r"(r3), "r"(r4) : "memory");
    return r2;
#else
#error "no system-call stub for this CPU"
#endif
}

// Linux system-call numbers (write, exit) per ABI: unverified, see the chapter.
#if defined(__x86_64__)
constexpr long kWrite = 1, kExit = 60;
#elif defined(__mips__)
constexpr long kWrite = 4004, kExit = 4001;   // o32 numbers start at 4000
#elif defined(__aarch64__) || defined(__riscv) || defined(__loongarch__)
constexpr long kWrite = 64, kExit = 93;       // the "generic" table newer ports share
#else
constexpr long kWrite = 4, kExit = 1;         // i386, arm, powerpc, s390x: the old numbering
#endif

} // namespace

// The entry point: the kernel (here: qemu-user) gives us a valid stack and nothing else.
// Each _start keeps the stack aligned as its ABI requires and calls ustart_main.
#if defined(__x86_64__)
asm(".globl _start\n_start:\n  xor %rbp, %rbp\n  and $-16, %rsp\n  call ustart_main\n  hlt\n");
#elif defined(__i386__)
asm(".globl _start\n_start:\n  xor %ebp, %ebp\n  and $-16, %esp\n  call ustart_main\n  hlt\n");
#elif defined(__aarch64__) || defined(__arm__)
asm(".globl _start\n_start:\n  bl ustart_main\n");
#elif defined(__riscv)
asm(".globl _start\n_start:\n  call ustart_main\n");
#elif defined(__loongarch__)
asm(".globl _start\n_start:\n  bl ustart_main\n");
#elif defined(__mips__)
asm(".globl _start\n.set noreorder\n_start:\n  addiu $sp, $sp, -32\n  jal ustart_main\n  nop\n.set reorder\n");
#elif defined(__powerpc__)
asm(".globl _start\n_start:\n  stwu 1, -16(1)\n  bl ustart_main\n");
#elif defined(__s390x__)
asm(".globl _start\n_start:\n  aghi %r15, -160\n  brasl %r14, ustart_main\n");
#endif

namespace serial {
void init() {}
void put(char c) { sys2(kWrite, 1, reinterpret_cast<long>(&c), 1); }
void write(const char* s)
{
    while (*s != '\0') {
        put(*s++);
    }
}
} // namespace serial

namespace arch {
[[noreturn]] void halt_forever()
{
    for (;;) {
        sys2(kExit, 2, 0, 0);
    }
}
[[noreturn]] void qemu_exit(uint8_t code)
{
    for (;;) {
        sys2(kExit, code == kExitPass ? 0 : 1, 0, 0);
    }
}
} // namespace arch

// The devicetree the tests parse: F4-27's pi3.dtb, embedded at build time (dtb_blob.h is
// written by run.sh). The parser must read it identically on little- and big-endian CPUs.
#include "dtb_blob.h"

extern "C" [[noreturn]] void ustart_main()
{
    run_global_constructors();
    NeutralEnv env{arch::kName, kDtbBlob, arch::kPageSize};
    int failures = run_neutral_tests(env);
    arch::qemu_exit(failures == 0 ? arch::kExitPass : arch::kExitFail);
}
