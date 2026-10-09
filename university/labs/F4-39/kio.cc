// kio.cc - serial console, a small printf, the Multiboot command line, QEMU exit.
#include "kio.h"
#include <stdarg.h>

namespace {
constexpr uint16_t COM1 = 0x3F8;
char g_cmdline[256];
}

void serial_init()
{
    outb(COM1 + 1, 0x00);   // no UART interrupts: the kernel polls
    outb(COM1 + 3, 0x80);   // DLAB on, to set the divisor
    outb(COM1 + 0, 0x01);   // divisor 1
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);   // 8 data bits, no parity, 1 stop bit; DLAB off
    outb(COM1 + 2, 0xC7);   // FIFO on and cleared
}

static void putc(char c)
{
    if (c == '\n') {
        putc('\r');
    }
    while ((inb(COM1 + 5) & 0x20) == 0) {   // line status: transmit holding register empty
    }
    outb(COM1, static_cast<uint8_t>(c));
}

void kputs(const char* s)
{
    while (*s) {
        putc(*s++);
    }
}

static void put_num(uint64_t v, unsigned base, int width, char pad, bool left, bool neg)
{
    char buf[24];
    int n = 0;
    do {
        buf[n++] = "0123456789abcdef"[v % base];
        v /= base;
    } while (v != 0);
    if (neg) {
        buf[n++] = '-';
    }
    int fill = width > n ? width - n : 0;
    if (!left) {
        for (int i = 0; i < fill; ++i) {
            putc(pad);
        }
    }
    while (n > 0) {
        putc(buf[--n]);
    }
    if (left) {
        for (int i = 0; i < fill; ++i) {
            putc(' ');
        }
    }
}

void kprintf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    for (; *fmt; ++fmt) {
        if (*fmt != '%') {
            putc(*fmt);
            continue;
        }
        ++fmt;
        bool left = false;
        char pad = ' ';
        int width = 0;
        if (*fmt == '-') { left = true; ++fmt; }
        if (*fmt == '0') { pad = '0'; ++fmt; }
        while (*fmt >= '0' && *fmt <= '9') { width = width * 10 + (*fmt++ - '0'); }
        bool is64 = false;
        if (*fmt == 'l') { is64 = true; ++fmt; }
        switch (*fmt) {
        case 's': {
            const char* s = va_arg(ap, const char*);
            int len = static_cast<int>(kstrlen(s));
            if (!left) { for (int i = len; i < width; ++i) { putc(' '); } }
            kputs(s);
            if (left) { for (int i = len; i < width; ++i) { putc(' '); } }
            break;
        }
        case 'c': putc(static_cast<char>(va_arg(ap, int))); break;
        case 'd': {
            int64_t v = is64 ? va_arg(ap, int64_t) : va_arg(ap, int);
            put_num(v < 0 ? -static_cast<uint64_t>(v) : static_cast<uint64_t>(v), 10, width, pad,
                    left, v < 0);
            break;
        }
        case 'u': put_num(is64 ? va_arg(ap, uint64_t) : va_arg(ap, unsigned), 10, width, pad, left, false); break;
        case 'x': put_num(is64 ? va_arg(ap, uint64_t) : va_arg(ap, unsigned), 16, width, pad, left, false); break;
        case '%': putc('%'); break;
        default: putc('?'); break;
        }
    }
    va_end(ap);
}

void halt_forever()
{
    for (;;) {
        asm volatile("cli; hlt");
    }
}

void qemu_exit(uint8_t code)
{
    outb(0xF4, code);   // isa-debug-exit: QEMU ends with status (code << 1) | 1
    halt_forever();     // without that device (real hardware), stop here
}

void cmdline_init(uint32_t magic, uint32_t info)
{
    g_cmdline[0] = 0;
    if (magic != 0x2BADB002 || info == 0) {
        return;
    }
    const uint32_t* mbi = reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(info));
    if ((mbi[0] & (1u << 2)) == 0) {   // flags bit 2: cmdline field (offset 16) is valid
        return;
    }
    const char* src = reinterpret_cast<const char*>(static_cast<uintptr_t>(mbi[4]));
    size_t i = 0;
    while (src[i] && i + 1 < sizeof g_cmdline) {
        g_cmdline[i] = src[i];
        ++i;
    }
    g_cmdline[i] = 0;
}

const char* cmdline() { return g_cmdline; }

static const char* find_value(const char* key)
{
    size_t klen = kstrlen(key);
    for (const char* p = g_cmdline; *p; ++p) {
        bool at_word = (p == g_cmdline || p[-1] == ' ');
        if (at_word && memcmp(p, key, klen) == 0 && p[klen] == '=') {
            return p + klen + 1;
        }
    }
    return nullptr;
}

bool arg_is(const char* key, const char* value)
{
    const char* v = find_value(key);
    if (v == nullptr) {
        return false;
    }
    size_t n = kstrlen(value);
    return memcmp(v, value, n) == 0 && (v[n] == 0 || v[n] == ' ');
}

uint64_t arg_num(const char* key, uint64_t fallback)
{
    const char* v = find_value(key);
    if (v == nullptr || *v < '0' || *v > '9') {
        return fallback;
    }
    uint64_t n = 0;
    while (*v >= '0' && *v <= '9') {
        n = n * 10 + static_cast<uint64_t>(*v++ - '0');
    }
    return n;
}

size_t kstrlen(const char* s)
{
    size_t n = 0;
    while (s[n]) {
        ++n;
    }
    return n;
}

extern "C" void* memcpy(void* d, const void* s, size_t n)
{
    auto* dp = static_cast<unsigned char*>(d);
    auto* sp = static_cast<const unsigned char*>(s);
    for (size_t i = 0; i < n; ++i) {
        dp[i] = sp[i];
    }
    return d;
}

extern "C" void* memset(void* d, int c, size_t n)
{
    auto* dp = static_cast<unsigned char*>(d);
    for (size_t i = 0; i < n; ++i) {
        dp[i] = static_cast<unsigned char>(c);
    }
    return d;
}

extern "C" int memcmp(const void* a, const void* b, size_t n)
{
    auto* ap = static_cast<const unsigned char*>(a);
    auto* bp = static_cast<const unsigned char*>(b);
    for (size_t i = 0; i < n; ++i) {
        if (ap[i] != bp[i]) {
            return ap[i] < bp[i] ? -1 : 1;
        }
    }
    return 0;
}
