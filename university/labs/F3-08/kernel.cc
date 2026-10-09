// kernel.cc - uni-rv, the university's teaching kernel for QEMU's RISC-V virt machine.
// One user process, a system-call table, and a check on every pointer the user passes in.
// F3-08 version: adds the system call "count" (the chapter's worked example).
#include <cstdint>

using u64 = std::uint64_t;

extern "C" char user_start[], user_end[], user_stack_top[], stacks[];
extern "C" void trap_entry();

// ---------------------------------------------------------------- devices (kernel only)
namespace {
volatile std::uint8_t* const uart = reinterpret_cast<volatile std::uint8_t*>(0x10000000);
volatile std::uint32_t* const finisher = reinterpret_cast<volatile std::uint32_t*>(0x100000);

void putc(char c) { *uart = static_cast<std::uint8_t>(c); }
void puts(const char* s)
{
    while (*s != '\0') { putc(*s++); }
}
void putdec(long v)
{
    if (v < 0) {
        putc('-');
        v = -v;
    }
    char buf[24];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v != 0);
    while (n > 0) { putc(buf[--n]); }
}
void puthex(u64 v)
{
    puts("0x");
    for (int shift = 60; shift >= 0; shift -= 4) { putc("0123456789abcdef"[(v >> shift) & 0xf]); }
}
[[noreturn]] void power_off(int code)
{
    *finisher = (code == 0) ? 0x5555u : ((static_cast<std::uint32_t>(code) << 16) | 0x3333u);
    for (;;) {}
}

// Kernel-only data: no user program should ever be able to read this.
const char kernel_note[] = "KERNEL ONLY: the exam answers are in block 7\n";

// ---------------------------------------------------------------- the process
struct Process {
    int pid = 1;
    long syscalls = 0;  // how many system calls it has made
};
Process current;

// ---------------------------------------------------------------- system calls
// Register convention: a7 = system-call number, a0, a1 = arguments, result back in a0.
enum : int { SYS_write = 1, SYS_getpid = 2, SYS_exit = 3, SYS_count = 4 };
using Frame = u64[32];  // saved registers, indexed by register number (x10 = a0)

bool user_range_ok(u64 addr, u64 len)
{
    const u64 lo = reinterpret_cast<u64>(user_start);
    const u64 hi = reinterpret_cast<u64>(user_end);
    return addr >= lo && len <= hi - lo && addr <= hi - len;
}

long sys_write(Frame& f)
{
    const u64 addr = f[10];
    const u64 len = f[11];
    if (!user_range_ok(addr, len)) { return -1; }  // never trust a user pointer
    const char* p = reinterpret_cast<const char*>(addr);
    for (u64 i = 0; i < len; ++i) { putc(p[i]); }
    return static_cast<long>(len);
}

long sys_getpid(Frame&) { return current.pid; }

long sys_count(Frame&) { return current.syscalls; }  // how many calls so far, this one included

long sys_exit(Frame& f)
{
    puts("[kernel] pid ");
    putdec(current.pid);
    puts(" syscall ");
    putdec(static_cast<long>(f[17]));
    puts(" (exit): status ");
    putdec(static_cast<long>(f[10]));
    puts(", ");
    putdec(current.syscalls);
    puts(" system calls in total; powering off\n");
    power_off(0);
}

struct Syscall {
    const char* name;
    long (*fn)(Frame&);
};
#ifndef BUGGY_TABLE
const Syscall syscalls[] = {
    // index = system-call number
    {"none", nullptr},  {"write", sys_write}, {"getpid", sys_getpid},
    {"exit", sys_exit}, {"count", sys_count},
};
#else
const Syscall syscalls[] = {
    // the forensic lab's version
    {"none", nullptr},      {"write", sys_write}, {"count", sys_count},
    {"getpid", sys_getpid}, {"exit", sys_exit},
};
#endif
constexpr long nsyscalls = sizeof(syscalls) / sizeof(syscalls[0]);

u64 read_mcause()
{
    u64 v;
    asm volatile("csrr %0, mcause" : "=r"(v));
    return v;
}
u64 read_mepc()
{
    u64 v;
    asm volatile("csrr %0, mepc" : "=r"(v));
    return v;
}
u64 read_mtval()
{
    u64 v;
    asm volatile("csrr %0, mtval" : "=r"(v));
    return v;
}
void write_mepc(u64 v) { asm volatile("csrw mepc, %0" : : "r"(v)); }
}  // namespace

// ---------------------------------------------------------------- traps
extern "C" void handle_trap(Frame& f)
{
    const u64 cause = read_mcause();
    if (cause == 8) {  // environment call (ecall) from user mode
        const long num = static_cast<long>(f[17]);
        ++current.syscalls;
        long result = -1;
        if (num > 0 && num < nsyscalls) { result = syscalls[num].fn(f); }
        puts("[kernel] pid ");
        putdec(current.pid);
        puts(" syscall ");
        putdec(num);
        puts(" (");
        puts(num > 0 && num < nsyscalls ? syscalls[num].name : "unknown");
        puts(") -> ");
        putdec(result);
        putc('\n');
        f[10] = static_cast<u64>(result);  // the result goes back in a0
        write_mepc(read_mepc() + 4);       // continue after the ecall instruction
        return;
    }
    puts("[kernel] user fault: mcause=");
    putdec(static_cast<long>(cause));
    puts(" mepc=");
    puthex(read_mepc());
    puts(" mtval=");
    puthex(read_mtval());
    puts("\n[kernel] process ");
    putdec(current.pid);
    puts(" killed\n");
    power_off(1);
}

// ---------------------------------------------------------------- the user program
// Everything below lives in the user region; it cannot see kernel code, data or devices.
#define USER_TEXT __attribute__((section(".user.text")))
#define USER_DATA __attribute__((section(".user.data")))

USER_DATA const char msg_hello[] = "user: hello from user mode\n";
USER_DATA const char msg_refused[] = "user: the kernel refused my pointer into kernel memory\n";
USER_DATA const char msg_done[] = "user: all done, exiting\n";

__attribute__((always_inline)) inline long ecall(long num, long a0, long a1)
{
    register long r0 asm("a0") = a0;
    register long r1 asm("a1") = a1;
    register long r7 asm("a7") = num;
    asm volatile("ecall" : "+r"(r0) : "r"(r1), "r"(r7) : "memory");
    return r0;
}

extern "C" USER_TEXT void user_main()
{
    ecall(SYS_write, reinterpret_cast<long>(msg_hello), sizeof msg_hello - 1);
    ecall(SYS_getpid, 0, 0);
    // Ask the kernel to print its own secret for us (we know its address from the ELF file):
    if (ecall(SYS_write, reinterpret_cast<long>(kernel_note), sizeof kernel_note - 1) == -1) {
        ecall(SYS_write, reinterpret_cast<long>(msg_refused), sizeof msg_refused - 1);
    }
#ifdef READ_HARTID_DIRECTLY
    long hart = 0;
    asm volatile("csrr %0, mhartid" : "=r"(hart));  // a machine-mode register, from user mode
#endif
    ecall(SYS_count, 0, 0);
    ecall(SYS_write, reinterpret_cast<long>(msg_done), sizeof msg_done - 1);
    ecall(SYS_exit, 0, 0);
    for (;;) {}
}

// ---------------------------------------------------------------- boot
extern "C" [[noreturn]] void enter_user(u64 pc, u64 sp);

extern "C" void kmain(u64 hart)
{
    if (hart != 0) {
        for (;;) { asm volatile("wfi"); }
    }  // this kernel uses one hart
    puts("[kernel] uni-rv booting on hart 0\n");
    const u64 base = reinterpret_cast<u64>(user_start);
    const u64 size = reinterpret_cast<u64>(user_end) - base;
    // PMP entry 0: user mode may read, write and execute the user region only (NAPOT).
    const u64 pmpaddr = (base + size / 2 - 1) >> 2;
    asm volatile("csrw pmpaddr0, %0" : : "r"(pmpaddr));
    asm volatile("csrw pmpcfg0, %0" : : "r"(0x1fUL));
    asm volatile("csrw mtvec, %0" : : "r"(&trap_entry));
    // The trap handler runs on hart 0's kernel stack.
    asm volatile("csrw mscratch, %0" : : "r"(stacks + 4096));
    puts("[kernel] user region ");
    puthex(base);
    puts("..");
    puthex(base + size);
    puts(", entering user mode\n");
    enter_user(reinterpret_cast<u64>(&user_main), reinterpret_cast<u64>(user_stack_top));
}
