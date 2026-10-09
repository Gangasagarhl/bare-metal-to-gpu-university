// console.h: output for the lab kernels. QEMU's debug console (I/O port 0xe9) prints every
// byte written to it; isa-debug-exit (port 0xf4) ends QEMU with status (value << 1) | 1.
#pragma once
#include <stdint.h>

inline void outb(uint16_t port, uint8_t value)
{
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

inline void print(const char* text)
{
    for (const char* p = text; *p != '\0'; ++p) {
        outb(0xe9, static_cast<uint8_t>(*p));
    }
}

inline void printHex(uint32_t value)
{
    print("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        outb(0xe9, static_cast<uint8_t>("0123456789abcdef"[(value >> shift) & 0xf]));
    }
}

inline void printDec(uint32_t value)
{
    char digits[11];
    int n = 0;
    do {
        digits[n++] = static_cast<char>('0' + value % 10);
        value /= 10;
    } while (value != 0);
    while (n > 0) {
        outb(0xe9, static_cast<uint8_t>(digits[--n]));
    }
}

[[noreturn]] inline void qemuExit(uint8_t code)
{
    outb(0xf4, code);
    for (;;) {
        asm volatile("hlt");
    }
}
