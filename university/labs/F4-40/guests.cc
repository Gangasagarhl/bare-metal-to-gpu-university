// guests.cc - tiny guest programs for F4-40. They run in guest mode (VMX non-root /
// SVM guest), in long mode, on the host's page tables. Every port access traps to the
// hypervisor (the I/O permission map has every bit set), so these "drivers" talk to the
// hypervisor's device models, not to QEMU's devices.
#include <stdint.h>

namespace {
inline void out(uint16_t port, uint8_t v) { asm volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
inline uint8_t in(uint16_t port)
{
    uint8_t v;
    asm volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
void puts(const char* s)
{
    for (; *s; ++s) {
        while ((in(0x3FD) & 0x20) == 0) {    // wait until the UART can take a byte
        }
        out(0x3F8, static_cast<uint8_t>(*s));
    }
}
void put_dec(uint64_t v)
{
    char buf[24];
    int n = 0;
    do { buf[n++] = static_cast<char>('0' + v % 10); v /= 10; } while (v != 0);
    char s[2] = {0, 0};
    while (n > 0) { s[0] = buf[--n]; puts(s); }
}
[[noreturn]] void hv_exit(uint64_t code)   // hypercall 0 of the DR404 hypervisor
{
    asm volatile("vmmcall" : : "a"(0ull), "b"(code));
    for (;;) {
    }
}
void pit_start_100hz()
{
    out(0x43, 0x34);                         // channel 0, lo/hi byte, mode 2
    out(0x40, 11932 & 0xFF);                 // 1193182 / 100, rounded
    out(0x40, 11932 >> 8);
}
uint16_t pit_read()
{
    out(0x43, 0x00);                         // latch channel 0
    uint8_t lo = in(0x40);
    uint8_t hi = in(0x40);
    return static_cast<uint16_t>(lo | (hi << 8));
}
}

extern "C" [[noreturn]] void guest_hello()
{
    puts("hello from a 64-bit guest\n");
    uint32_t a, b, c, d;
    asm volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(0x40000000u), "c"(0u));
    char sig[13];
    for (int i = 0; i < 4; ++i) {
        sig[i] = static_cast<char>(b >> (8 * i));
        sig[4 + i] = static_cast<char>(c >> (8 * i));
        sig[8 + i] = static_cast<char>(d >> (8 * i));
    }
    sig[12] = 0;
    puts("my hypervisor says it is \"");
    puts(sig);
    puts("\"\n");
    hv_exit(0);
}

extern "C" [[noreturn]] void guest_triple()
{
    puts("about to execute ud2 with no IDT\n");
    asm volatile("ud2");                     // #UD -> cannot deliver -> #DF -> triple fault
    hv_exit(1);
}

// The forensic guest: waits 2 seconds by spinning on the PIT counter.
extern "C" [[noreturn]] void guest_busy()
{
    pit_start_100hz();
    uint64_t wraps = 0;
    uint64_t polls = 0;
    uint16_t last = pit_read();
    while (wraps < 200) {                    // 200 ticks of 10 ms
        uint16_t now = pit_read();
        if (now > last) { ++wraps; }
        last = now;
        ++polls;
    }
    puts("busy guest: waited 200 ticks, PIT polls: ");
    put_dec(polls);
    puts("\n");
    hv_exit(0);
}

// The well-behaved guest: halts and lets the hypervisor wake it at each tick.
extern "C" [[noreturn]] void guest_hlt()
{
    pit_start_100hz();
    asm volatile("sti");
    for (int i = 0; i < 200; ++i) {
        asm volatile("hlt");
    }
    puts("halting guest: waited 200 ticks\n");
    hv_exit(0);
}
