// kbase.cc - DR403 kernel base (see kbase.h).
#include "kbase.h"

#include <stdarg.h>

namespace {
k::PutcFn g_putc = nullptr;

void put(char c)
{
    if (g_putc != nullptr) {
        if (c == '\n') {
            g_putc('\r');
        }
        g_putc(c);
    }
}

void put_num(uint64_t v, unsigned base, int width, char pad, bool neg)
{
    char buf[24];
    int n = 0;
    do {
        unsigned d = static_cast<unsigned>(v % base);
        buf[n++] = static_cast<char>(d < 10 ? '0' + d : 'a' + d - 10);
        v /= base;
    } while (v != 0);
    if (neg) {
        buf[n++] = '-';
    }
    for (int i = n; i < width; ++i) {
        put(pad);
    }
    while (n > 0) {
        put(buf[--n]);
    }
}
}  // namespace

namespace k {

void set_console(PutcFn fn) { g_putc = fn; }

// Supports %s %c %d %u %x %p with an optional '0' flag, a width and the l / ll length modifiers.
void printf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    for (const char* p = fmt; *p != '\0'; ++p) {
        if (*p != '%') {
            put(*p);
            continue;
        }
        ++p;
        char pad = ' ';
        int width = 0;
        int longs = 0;
        bool left = false;
        if (*p == '-') {
            left = true;
            ++p;
        }
        if (*p == '0') {
            pad = '0';
            ++p;
        }
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p++ - '0');
        }
        while (*p == 'l') {
            ++longs;
            ++p;
        }
        switch (*p) {
        case 's': {
            const char* s = va_arg(ap, const char*);
            int len = 0;
            while (s[len] != '\0') {
                ++len;
            }
            if (!left) {
                for (int i = len; i < width; ++i) {
                    put(' ');
                }
            }
            for (int i = 0; i < len; ++i) {
                put(s[i]);
            }
            if (left) {
                for (int i = len; i < width; ++i) {
                    put(' ');
                }
            }
            break;
        }
        case 'c':
            put(static_cast<char>(va_arg(ap, int)));
            break;
        case 'd': {
            int64_t v = longs ? va_arg(ap, int64_t) : va_arg(ap, int);
            put_num(v < 0 ? static_cast<uint64_t>(-v) : static_cast<uint64_t>(v), 10, width, pad, v < 0);
            break;
        }
        case 'u':
            put_num(longs ? va_arg(ap, uint64_t) : va_arg(ap, unsigned), 10, width, pad, false);
            break;
        case 'x':
            put_num(longs ? va_arg(ap, uint64_t) : va_arg(ap, unsigned), 16, width, pad, false);
            break;
        case 'p':
            put('0');
            put('x');
            put_num(reinterpret_cast<uint64_t>(va_arg(ap, void*)), 16, width, pad, false);
            break;
        case '%':
            put('%');
            break;
        default:
            put('?');
            break;
        }
    }
    va_end(ap);
}

void delay_us(uint64_t us)
{
    uint64_t end = counter() + us * counter_freq() / 1000000;
    while (counter() < end) {
    }
}

// Semihosting SYS_EXIT (operation 0x18) with the reason "application exit" (0x20026) and an
// exit code, through HLT #0xF000. Numbers after the Arm semihosting specification (title only,
// pending verification); QEMU needs -semihosting. The exit code QEMU returns is the lab's check.
void exit(int code)
{
    static uint64_t block[2];
    block[0] = 0x20026;
    block[1] = static_cast<uint64_t>(code);
    register uint64_t x0 asm("x0") = 0x18;
    register uint64_t x1 asm("x1") = reinterpret_cast<uint64_t>(block);
    asm volatile("hlt #0xf000" : : "r"(x0), "r"(x1) : "memory");
    while (true) {
        asm volatile("wfe");
    }
}

}  // namespace k

// Called by every entry of the vector table in start.S. ESR_EL1 bits 31:26 are the exception
// class; FAR_EL1 holds the faulting address for aborts (Arm Architecture Reference Manual for
// A-profile, exception syndrome register description - title only, pending verification).
extern "C" [[noreturn]] void exception_report(uint64_t idx, uint64_t esr, uint64_t elr, uint64_t far)
{
    using ull = unsigned long long;
    k::printf("EXCEPTION vector %u: ESR_EL1=0x%llx (class 0x%x) ELR_EL1=0x%llx FAR_EL1=0x%llx\n",
              static_cast<unsigned>(idx), ull{esr}, static_cast<unsigned>((esr >> 26) & 0x3f), ull{elr}, ull{far});
    k::exit(3);
}

// The compiler may call these for struct copies and zeroing; byte loops are always aligned.
extern "C" void* memset(void* d, int c, size_t n)
{
    auto* p = static_cast<unsigned char*>(d);
    for (size_t i = 0; i < n; ++i) {
        p[i] = static_cast<unsigned char>(c);
    }
    return d;
}
extern "C" void* memcpy(void* d, const void* s, size_t n)
{
    auto* p = static_cast<unsigned char*>(d);
    const auto* q = static_cast<const unsigned char*>(s);
    for (size_t i = 0; i < n; ++i) {
        p[i] = q[i];
    }
    return d;
}
