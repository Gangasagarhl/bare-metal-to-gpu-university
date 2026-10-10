// k.cc - DS403 cluster kernel: serial console, printing, PIT clock, command line.
#include "k.h"
#include <stdarg.h>

uint8_t g_node = 0;

namespace {
constexpr uint16_t COM1 = 0x3F8;

void serial_putc(char c)
{
    while ((inb(COM1 + 5) & 0x20) == 0) { }      // line status: transmit holding register empty
    outb(COM1, static_cast<uint8_t>(c));
}

void put_uint(uint32_t v, unsigned base, int width, char pad)
{
    char buf[12];
    int n = 0;
    do {
        unsigned d = v % base;
        buf[n++] = static_cast<char>(d < 10 ? '0' + d : 'a' + d - 10);
        v /= base;
    } while (v != 0);
    while (n < width) buf[n++] = pad;
    while (n > 0) serial_putc(buf[--n]);
}

void vprint(const char* fmt, va_list ap)
{
    for (const char* p = fmt; *p != '\0'; ++p) {
        if (*p != '%') {
            if (*p == '\n') serial_putc('\r');
            serial_putc(*p);
            continue;
        }
        ++p;
        char pad = ' ';
        if (*p == '0') { pad = '0'; ++p; }
        int width = 0;
        while (*p >= '0' && *p <= '9') width = width * 10 + (*p++ - '0');
        switch (*p) {
        case 's': for (const char* s = va_arg(ap, const char*); *s != '\0'; ++s) serial_putc(*s); break;
        case 'c': serial_putc(static_cast<char>(va_arg(ap, int))); break;
        case 'u': put_uint(va_arg(ap, uint32_t), 10, width, pad); break;
        case 'x': put_uint(va_arg(ap, uint32_t), 16, width, pad); break;
        case 'd': {
            int32_t v = va_arg(ap, int32_t);
            if (v < 0) { serial_putc('-'); v = -v; }
            put_uint(static_cast<uint32_t>(v), 10, width, pad);
            break;
        }
        default: serial_putc('%'); serial_putc(*p); break;
        }
    }
}

// PIT channel 0 counts down from RELOAD to 1 at the PIT input clock and starts again.
// RELOAD = 11932 gives about 10 ms per period (see the unverified box in F5-44).
constexpr uint16_t RELOAD = 11932;
uint16_t last_count = 0;
uint32_t ticks = 0;

uint16_t pit_read()
{
    outb(0x43, 0x00);                            // latch the count of channel 0
    uint8_t lo = inb(0x40);
    uint8_t hi = inb(0x40);
    return static_cast<uint16_t>(lo | (hi << 8));
}

char cmdline[256];
char loader[64];
uint32_t rnd = 1;
}  // namespace

void serial_init()
{
    outb(COM1 + 1, 0x00);   // no UART interrupts
    outb(COM1 + 3, 0x80);   // DLAB on: next two writes set the divisor
    outb(COM1 + 0, 0x01);   // divisor 1 (the fastest rate)
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);   // 8 data bits, no parity, 1 stop bit, DLAB off
    outb(COM1 + 2, 0xC7);   // FIFOs on and cleared
}

void kprintf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
}

void klog(const char* fmt, ...)
{
    kprintf("[t=%06u n%u] ", now(), g_node);
    va_list ap;
    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
    kprintf("\n");
}

void qemu_exit(uint8_t code)
{
    outb(0xF4, code);
    for (;;) asm volatile("hlt");
}

void clock_init()
{
    outb(0x43, 0x34);                            // channel 0, low then high byte, mode 2
    outb(0x40, RELOAD & 0xFF);
    outb(0x40, RELOAD >> 8);
    last_count = pit_read();
    ticks = 0;
}

uint32_t now()
{
    // The count goes down; when it is larger than last time, one period has ended.
    // Callers poll far more often than every 10 ms, so at most one wrap is missed rarely.
    uint16_t c = pit_read();
    if (c > last_count) ++ticks;
    last_count = c;
    return ticks;
}

void cmdline_init(uint32_t magic, uint32_t mbinfo)
{
    cmdline[0] = '\0';
    loader[0] = '\0';
    if (magic != 0x2BADB002) return;
    const uint32_t* info = reinterpret_cast<const uint32_t*>(mbinfo);
    if (info[0] & (1u << 2))                      // flags bit 2: cmdline field is valid
        kstrlcpy(cmdline, reinterpret_cast<const char*>(info[4]), sizeof cmdline);
    if (info[0] & (1u << 9))                      // flags bit 9: boot_loader_name is valid
        kstrlcpy(loader, reinterpret_cast<const char*>(info[16]), sizeof loader);
}

static const char* find_value(const char* key)
{
    size_t kl = kstrlen(key);
    for (const char* p = cmdline; *p != '\0'; ++p) {
        bool at_word = (p == cmdline || p[-1] == ' ');
        if (!at_word) continue;
        size_t i = 0;
        while (i < kl && p[i] == key[i]) ++i;      // stops at the end of the line too
        if (i == kl && p[kl] == '=') return p + kl + 1;
    }
    return nullptr;
}

uint32_t arg_u32(const char* key, uint32_t fallback)
{
    const char* v = find_value(key);
    if (v == nullptr || *v < '0' || *v > '9') return fallback;
    uint32_t n = 0;
    while (*v >= '0' && *v <= '9') n = n * 10 + static_cast<uint32_t>(*v++ - '0');
    return n;
}

bool arg_is(const char* key, const char* value)
{
    const char* v = find_value(key);
    if (v == nullptr) return false;
    size_t i = 0;
    while (value[i] != '\0' && v[i] == value[i]) ++i;
    return value[i] == '\0' && (v[i] == ' ' || v[i] == '\0');
}

const char* boot_loader_name() { return loader; }

void rand_seed(uint32_t s) { rnd = s != 0 ? s : 1; }

uint32_t rand_u32()
{
    rnd ^= rnd << 13;
    rnd ^= rnd >> 17;
    rnd ^= rnd << 5;
    return rnd;
}

extern "C" void* memcpy(void* d, const void* s, size_t n)
{
    auto* dp = static_cast<uint8_t*>(d);
    auto* sp = static_cast<const uint8_t*>(s);
    while (n-- > 0) *dp++ = *sp++;
    return d;
}

extern "C" void* memset(void* d, int c, size_t n)
{
    auto* dp = static_cast<uint8_t*>(d);
    while (n-- > 0) *dp++ = static_cast<uint8_t>(c);
    return d;
}

extern "C" int memcmp(const void* a, const void* b, size_t n)
{
    auto* x = static_cast<const uint8_t*>(a);
    auto* y = static_cast<const uint8_t*>(b);
    for (size_t i = 0; i < n; ++i)
        if (x[i] != y[i]) return x[i] < y[i] ? -1 : 1;
    return 0;
}

size_t kstrlen(const char* s)
{
    size_t n = 0;
    while (s[n] != '\0') ++n;
    return n;
}

int kstrcmp(const char* a, const char* b)
{
    while (*a != '\0' && *a == *b) { ++a; ++b; }
    return static_cast<uint8_t>(*a) - static_cast<uint8_t>(*b);
}

void kstrlcpy(char* d, const char* s, size_t cap)
{
    size_t i = 0;
    for (; i + 1 < cap && s[i] != '\0'; ++i) d[i] = s[i];
    if (cap > 0) d[i] = '\0';
}
