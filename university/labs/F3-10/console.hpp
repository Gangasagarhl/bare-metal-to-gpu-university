// console.hpp - a tiny C++ wrapper around the UEFI text console (no exceptions, no RTTI,
// no heap). Converts 8-bit text to the UCS-2 the console protocol expects, adds "\r"
// before every "\n", and prints numbers in decimal or hexadecimal.
#pragma once
#include "efi.hpp"

class Console
{
public:
    explicit Console(efi::SimpleTextOutput* out) : out_(out) {}

    void print(const char* text)
    {
        efi::Char16 buf[128];
        size_t n = 0;
        for (; *text != '\0'; ++text) {
            if (n + 3 >= sizeof buf / sizeof buf[0]) {
                flush(buf, n);
            }
            if (*text == '\n') {
                buf[n++] = u'\r';
            }
            buf[n++] = static_cast<efi::Char16>(static_cast<unsigned char>(*text));
        }
        flush(buf, n);
    }

    void dec(uint64_t value)
    {
        char digits[21];
        int i = 20;
        digits[i] = '\0';
        do {
            digits[--i] = static_cast<char>('0' + value % 10);
            value /= 10;
        } while (value != 0);
        print(&digits[i]);
    }

    void hex(uint64_t value, int width = 16)
    {
        char digits[19] = "0x";
        for (int i = width - 1; i >= 0; --i) {
            digits[2 + i] = "0123456789abcdef"[value & 0xf];
            value >>= 4;
        }
        digits[2 + width] = '\0';
        print(digits);
    }

private:
    void flush(efi::Char16* buf, size_t& n)
    {
        buf[n] = u'\0';
        out_->output_string(out_, buf);
        n = 0;
    }

    efi::SimpleTextOutput* out_;
};

// Ends a QEMU test run: writing to the isa-debug-exit device makes QEMU exit with
// status (value << 1) | 1. Only meaningful in QEMU started with
// -device isa-debug-exit,iobase=0xf4,iosize=0x04 (QEMU documentation; pending verification).
inline void qemu_exit(uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(static_cast<uint16_t>(0xf4)));
}
