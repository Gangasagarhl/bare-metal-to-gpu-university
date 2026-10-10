// mb_main.cc - what a Multiboot (version 1) loader hands over: the magic in EAX and a pointer
// to an information structure in EBX. Prints it on COM1 and ends the QEMU run.
// Structure offsets written from memory of the Multiboot Specification (pending verification).
#include <stdint.h>

namespace {

void outb(uint16_t port, uint8_t v) { __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
uint8_t inb(uint16_t port)
{
    uint8_t v;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

void putc(char c)
{
    while ((inb(0x3f8 + 5) & 0x20) == 0) {
    }
    outb(0x3f8, static_cast<uint8_t>(c));
}

void print(const char* s)
{
    for (; *s != '\0'; ++s) {
        if (*s == '\n') {
            putc('\r');
        }
        putc(*s);
    }
}

void hex(uint64_t v, int digits)
{
    print("0x");
    for (int i = digits - 1; i >= 0; --i) {
        putc("0123456789abcdef"[(v >> (4 * i)) & 0xf]);
    }
}

void dec(uint32_t v)
{
    char buf[11];
    int i = 10;
    buf[i] = '\0';
    do {
        buf[--i] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v != 0);
    print(&buf[i]);
}

uint32_t u32(uint32_t address) { return *reinterpret_cast<const volatile uint32_t*>(address); }
uint64_t u64(uint32_t address) { return u32(address) | static_cast<uint64_t>(u32(address + 4)) << 32; }

}  // namespace

extern "C" void mb_main(uint32_t magic, uint32_t info)
{
    // nobody set up the serial port for us: 8 data bits, no parity, 1 stop bit, divisor 1
    outb(0x3f8 + 1, 0x00);
    outb(0x3f8 + 3, 0x80);
    outb(0x3f8 + 0, 0x01);
    outb(0x3f8 + 1, 0x00);
    outb(0x3f8 + 3, 0x03);
    print("multiboot kernel entered in 32-bit protected mode\nEAX magic ");
    hex(magic, 8);
    print(magic == 0x2BADB002 ? " (the Multiboot loader value)\n" : " (NOT the Multiboot value)\n");
    const uint32_t flags = u32(info);
    print("info structure at ");
    hex(info, 8);
    print(", flags ");
    hex(flags, 8);
    print("\n");
    if (flags & (1u << 0)) {
        print("  memory: lower ");
        dec(u32(info + 4));
        print(" KiB, upper ");
        dec(u32(info + 8));
        print(" KiB\n");
    }
    if (flags & (1u << 2)) {
        print("  command line: '");
        print(reinterpret_cast<const char*>(u32(info + 16)));
        print("'\n");
    }
    if (flags & (1u << 9)) {
        print("  boot loader name: '");
        print(reinterpret_cast<const char*>(u32(info + 64)));
        print("'\n");
    }
    if (flags & (1u << 6)) {
        print("  memory map:\n");
        const uint32_t start = u32(info + 48), length = u32(info + 44);
        for (uint32_t e = start; e < start + length; e += u32(e) + 4) {
            print("    base ");
            hex(u64(e + 4), 16);
            print(" length ");
            hex(u64(e + 12), 16);
            print(" type ");
            hex(u32(e + 20), 1);
            print("\n");
        }
    }
    print("multiboot kernel: done\n");
    outb(0xf4, 0x10);  // QEMU isa-debug-exit: status (0x10 << 1) | 1 = 33
}
