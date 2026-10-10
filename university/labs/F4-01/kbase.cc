// kbase.cc - DR301 lab kernel: COM1 log output, formatted printing, memory helpers.
#include "kbase.h"
#include <stdarg.h>

static constexpr uint16_t COM1 = 0x3F8;  // the first PC serial port (16550-compatible UART)

void serial_init()
{
    outb(COM1 + 1, 0x00);                // IER: no interrupts, the log channel polls
    outb(COM1 + 3, 0x80);                // LCR: DLAB on, the next two writes set the divisor
    outb(COM1 + 0, 0x01);                // DLL: divisor 1
    outb(COM1 + 1, 0x00);                // DLM
    outb(COM1 + 3, 0x03);                // LCR: 8 data bits, no parity, 1 stop bit, DLAB off
    outb(COM1 + 2, 0x07);                // FCR: enable and clear both FIFOs
}

void serial_putc(char c)
{
    if (c == '\n') serial_putc('\r');
    while ((inb(COM1 + 5) & 0x20) == 0) { }   // LSR bit 5: transmit holding register empty
    outb(COM1, static_cast<uint8_t>(c));
}

void kputs(const char* s) { while (*s) serial_putc(*s++); }

// v /= base, returning the remainder, with 32-bit operations only: a 32-bit kernel has no
// libgcc helper for 64-bit division (__udivmoddi4), so divide 16 bits at a time.
static unsigned div_small(uint64_t& v, unsigned base)
{
    uint32_t r = 0;
    uint64_t q = 0;
    for (int shift = 48; shift >= 0; shift -= 16) {
        const uint32_t cur = (r << 16) | static_cast<uint32_t>((v >> shift) & 0xFFFF);
        q |= static_cast<uint64_t>(cur / base) << shift;
        r = cur % base;
    }
    v = q;
    return r;
}

static void put_num(uint64_t v, unsigned base, int width, char pad, bool neg, bool left)
{
    char buf[24];
    int n = 0;
    do { buf[n++] = "0123456789abcdef"[div_small(v, base)]; } while (v != 0);
    if (neg) buf[n++] = '-';
    int len = n;
    if (!left) while (n < width) buf[n++] = pad;
    while (n > 0) serial_putc(buf[--n]);
    if (left) for (int i = len; i < width; ++i) serial_putc(' ');
}

void kprintf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    for (; *fmt; ++fmt) {
        if (*fmt != '%') { serial_putc(*fmt); continue; }
        ++fmt;
        bool left = false;
        if (*fmt == '-') { left = true; ++fmt; }
        char pad = ' ';
        if (*fmt == '0') { pad = '0'; ++fmt; }
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        bool wide = false;
        if (*fmt == 'l') { wide = true; ++fmt; }
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
        case 'd': {
            int v = va_arg(ap, int);
            uint64_t mag = v < 0 ? 0u - static_cast<uint32_t>(v) : static_cast<uint32_t>(v);
            put_num(mag, 10, width, pad, v < 0, left);
            break;
        }
        case 'u':
            put_num(wide ? va_arg(ap, uint64_t) : va_arg(ap, uint32_t), 10, width, pad, false, left);
            break;
        case 'x':
            put_num(wide ? va_arg(ap, uint64_t) : va_arg(ap, uint32_t), 16, width, pad, false, left);
            break;
        case '%': serial_putc('%'); break;
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

void panic(const char* msg)
{
    kprintf("PANIC: %s\n", msg);
    qemu_exit(0x01);                     // QEMU exit status 3 = failure
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

extern "C" int memcmp(const void* a, const void* b, size_t n)
{
    auto* pa = static_cast<const unsigned char*>(a);
    auto* pb = static_cast<const unsigned char*>(b);
    for (size_t i = 0; i < n; ++i)
        if (pa[i] != pb[i]) return pa[i] - pb[i];
    return 0;
}

size_t kstrlen(const char* s) { size_t n = 0; while (s[n]) ++n; return n; }

int kstrcmp(const char* a, const char* b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return static_cast<unsigned char>(*a) - static_cast<unsigned char>(*b);
}

uint32_t fnv1a(const void* data, size_t n, uint32_t h)
{
    auto* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < n; ++i) { h ^= p[i]; h *= 16777619u; }
    return h;
}
