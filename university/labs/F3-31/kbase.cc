// kbase.cc - F3-31 mini kernel: COM1 serial port, formatted printing, string helpers.
#include "kbase.h"
#include <stdarg.h>

static constexpr uint16_t COM1 = 0x3F8;  // 16550 UART registers at COM1 (OSDev wiki "Serial Ports")

void serial_init()
{
    outb(COM1 + 1, 0x00);                // no interrupts: this kernel polls
    outb(COM1 + 3, 0x80);                // DLAB on: next two writes set the divisor
    outb(COM1 + 0, 0x01);                // divisor 1 = 115200 baud
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);                // 8 data bits, no parity, 1 stop bit, DLAB off
    outb(COM1 + 2, 0xC7);                // enable and clear the FIFOs
}

void serial_putc(char c)
{
    if (c == '\n') serial_putc('\r');
    while ((inb(COM1 + 5) & 0x20) == 0) { }   // line status bit 5: transmit holding empty
    outb(COM1, static_cast<uint8_t>(c));
}

int serial_getc()
{
    while ((inb(COM1 + 5) & 0x01) == 0) { }   // line status bit 0: data ready
    return inb(COM1);
}

void kputs(const char* s) { while (*s) serial_putc(*s++); }

static void put_num(uint32_t v, unsigned base, int width, char pad, bool neg)
{
    char buf[16];
    int n = 0;
    do { buf[n++] = "0123456789abcdef"[v % base]; v /= base; } while (v != 0);
    if (neg) buf[n++] = '-';
    while (n < width) buf[n++] = pad;
    while (n > 0) serial_putc(buf[--n]);
}

void kprintf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    for (; *fmt; ++fmt) {
        if (*fmt != '%') { serial_putc(*fmt); continue; }
        ++fmt;
        char pad = ' ';
        if (*fmt == '0') { pad = '0'; ++fmt; }
        bool left = false;
        if (*fmt == '-') { left = true; ++fmt; }
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        switch (*fmt) {
        case 's': {
            const char* s = va_arg(ap, const char*);
            int len = static_cast<int>(kstrlen(s));
            if (!left) for (int i = len; i < width; ++i) serial_putc(' ');
            kputs(s);
            if (left) for (int i = len; i < width; ++i) serial_putc(' ');
            break;
        }
        case 'c': serial_putc(static_cast<char>(va_arg(ap, int))); break;
        case 'd': { int v = va_arg(ap, int);
                    put_num(v < 0 ? 0u - static_cast<uint32_t>(v) : static_cast<uint32_t>(v), 10, width, pad, v < 0);
                    break; }
        case 'u': put_num(va_arg(ap, uint32_t), 10, width, pad, false); break;
        case 'x': put_num(va_arg(ap, uint32_t), 16, width, pad, false); break;
        case 'o': put_num(va_arg(ap, uint32_t), 8, width, pad, false); break;
        default: serial_putc('%'); serial_putc(*fmt); break;
        }
    }
    va_end(ap);
}

void qemu_exit(uint8_t code)
{
    outb(0xF4, code);                    // QEMU device isa-debug-exit,iobase=0xf4
    for (;;) asm volatile("cli; hlt");
}

extern "C" void* memcpy(void* d, const void* s, size_t n)
{
    auto* dp = static_cast<unsigned char*>(d);
    auto* sp = static_cast<const unsigned char*>(s);
    while (n--) *dp++ = *sp++;
    return d;
}

extern "C" void* memset(void* d, int c, size_t n)
{
    auto* dp = static_cast<unsigned char*>(d);
    while (n--) *dp++ = static_cast<unsigned char>(c);
    return d;
}

size_t kstrlen(const char* s) { size_t n = 0; while (s[n]) ++n; return n; }

int kstrcmp(const char* a, const char* b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return static_cast<unsigned char>(*a) - static_cast<unsigned char>(*b);
}

int kstrncmp(const char* a, const char* b, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i] || a[i] == 0)
            return static_cast<unsigned char>(a[i]) - static_cast<unsigned char>(b[i]);
    }
    return 0;
}

void kstrlcpy(char* d, const char* s, size_t cap)
{
    size_t i = 0;
    for (; i + 1 < cap && s[i]; ++i) d[i] = s[i];
    if (cap) d[i] = 0;
}
