// tlibc.cc - F3-35: the portable part of tlibc, a teaching C library. Nothing here knows which
// operating system it runs on: all contact with the kernel goes through the two functions of the
// system-call layer (sys_*.cc), which is the only file a port replaces.
#include <stdarg.h>
#include "tlibc_include/stdio.h"
#include "tlibc_include/stdlib.h"
#include "tlibc_include/string.h"

extern "C" long tlibc_sys_write(int fd, const void* buf, size_t n);   // system-call layer
extern "C" [[noreturn]] void tlibc_sys_exit(int status);
extern "C" int main(int argc, char** argv);

namespace {
char outbuf[256];                        // standard output is buffered; flushed when full,
size_t outlen = 0;                       // at every newline and at exit
int written = 0;                         // characters produced, for printf's return value

void flush()
{
    size_t done = 0;
    while (done < outlen) {
        long k = tlibc_sys_write(1, outbuf + done, outlen - done);
        if (k <= 0) break;               // nothing sensible to do on error in this tiny library
        done += static_cast<size_t>(k);
    }
    outlen = 0;
}

void put(char c)
{
    outbuf[outlen++] = c;
    ++written;
    if (outlen == sizeof outbuf || c == '\n') flush();
}

void pad_to(int len, int width, char pad) { for (int i = len; i < width; ++i) put(pad); }

void put_number(unsigned long v, unsigned base, int width, char pad, bool left, bool neg)
{
    char tmp[24];
    int n = 0;
    do { tmp[n++] = "0123456789abcdef"[v % base]; v /= base; } while (v);
    int len = n + (neg ? 1 : 0);
    if (neg && pad == '0') put('-');     // "-0042": the sign goes before zero padding
    if (!left) pad_to(len, width, pad);
    if (neg && pad != '0') put('-');
    while (n) put(tmp[--n]);
    if (left) pad_to(len, width, ' ');
}
} // namespace

extern "C" {

size_t strlen(const char* s) { size_t n = 0; while (s[n]) ++n; return n; }

int strcmp(const char* a, const char* b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return static_cast<unsigned char>(*a) - static_cast<unsigned char>(*b);
}

void* memcpy(void* d, const void* s, size_t n)
{
    auto* dp = static_cast<unsigned char*>(d);
    auto* sp = static_cast<const unsigned char*>(s);
    while (n--) *dp++ = *sp++;
    return d;
}

void* memset(void* d, int c, size_t n)
{
    auto* dp = static_cast<unsigned char*>(d);
    while (n--) *dp++ = static_cast<unsigned char>(c);
    return d;
}

int abs(int v) { return v < 0 ? -v : v; }

int putchar(int c) { put(static_cast<char>(c)); return c; }

int puts(const char* s)
{
    while (*s) put(*s++);
    put('\n');
    return 1;
}

int printf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int start = written;
    for (; *fmt; ++fmt) {
        if (*fmt != '%') { put(*fmt); continue; }
        ++fmt;
        bool left = false;
        char pad = ' ';
        for (;; ++fmt) {                 // flags
            if (*fmt == '-') left = true;
            else if (*fmt == '0') pad = '0';
            else break;
        }
        if (left) pad = ' ';
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        bool is_long = false;
        if (*fmt == 'l') { is_long = true; ++fmt; }
        switch (*fmt) {
        case 'd': {
            long v = is_long ? va_arg(ap, long) : va_arg(ap, int);
            unsigned long mag = v < 0 ? 0ul - static_cast<unsigned long>(v) : static_cast<unsigned long>(v);
            put_number(mag, 10, width, pad, left, v < 0);
            break;
        }
        case 'u': put_number(is_long ? va_arg(ap, unsigned long) : va_arg(ap, unsigned), 10, width, pad, left, false); break;
        case 'x': put_number(is_long ? va_arg(ap, unsigned long) : va_arg(ap, unsigned), 16, width, pad, left, false); break;
        case 'c': put(static_cast<char>(va_arg(ap, int))); break;
        case 's': {
            const char* s = va_arg(ap, const char*);
            int len = static_cast<int>(strlen(s));
            if (!left) pad_to(len, width, ' ');
            while (*s) put(*s++);
            if (left) pad_to(len, width, ' ');
            break;
        }
        case '%': put('%'); break;
        default: put('%'); put(*fmt); break;
        }
    }
    va_end(ap);
    return written - start;
}

[[noreturn]] void exit(int status)
{
    flush();
    tlibc_sys_exit(status);
}

// Called by the start-up code of the system-call layer with the initial stack pointer.
// The stack layout (argc, then the argv pointers) follows the System V ABI's process start-up.
[[noreturn]] void tlibc_start(long* sp)
{
    int argc = static_cast<int>(sp[0]);
    char** argv = reinterpret_cast<char**>(sp + 1);
    exit(main(argc, argv));
}

} // extern "C"
