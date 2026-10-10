// kformat.h - F3-18: a tiny printf-style formatter with no library underneath.
// Pure logic (no hardware), so the same file is unit-tested on the host (fmt_host.cpp).
// Supports %d %i %u %x %c %s %p %%, an optional '0' flag, a width, a precision for %s
// ("%.4s": at most 4 characters, for fixed-size fields such as ACPI signatures), and 'l'.
#pragma once
#include <cstdarg>
#include <cstdint>

using KSink = void (*)(char c, void* ctx);

namespace kfmt_detail {

inline void emit_number(KSink sink, void* ctx, uint64_t v, unsigned base, bool negative,
                        int width, char pad)
{
    char buf[24];
    int n = 0;
    do {
        unsigned digit = static_cast<unsigned>(v % base);
        buf[n++] = static_cast<char>(digit < 10 ? '0' + digit : 'a' + digit - 10);
        v /= base;
    } while (v != 0);
    int len = n + (negative ? 1 : 0);
    if (negative && pad == '0') {
        sink('-', ctx);
    }
    for (; len < width; ++len) {
        sink(pad, ctx);
    }
    if (negative && pad != '0') {
        sink('-', ctx);
    }
    while (n > 0) {
        sink(buf[--n], ctx);
    }
}

} // namespace kfmt_detail

inline void kvformat(KSink sink, void* ctx, const char* fmt, va_list ap)
{
    for (const char* p = fmt; *p != '\0'; ++p) {
        if (*p != '%') {
            sink(*p, ctx);
            continue;
        }
        ++p;
        char pad = ' ';
        int width = 0;
        bool is_long = false;
        if (*p == '0') {
            pad = '0';
            ++p;
        }
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p++ - '0');
        }
        int precision = -1;              // -1: no limit
        if (*p == '.') {
            precision = 0;
            ++p;
            while (*p >= '0' && *p <= '9') {
                precision = precision * 10 + (*p++ - '0');
            }
        }
        while (*p == 'l') {
            is_long = true;
            ++p;
        }
        switch (*p) {
        case 'd':
        case 'i': {
            int64_t v = is_long ? va_arg(ap, long) : va_arg(ap, int);
            uint64_t mag = v < 0 ? 0 - static_cast<uint64_t>(v) : static_cast<uint64_t>(v);
            kfmt_detail::emit_number(sink, ctx, mag, 10, v < 0, width, pad);
            break;
        }
        case 'u':
        case 'x': {
            uint64_t v = is_long ? va_arg(ap, unsigned long) : va_arg(ap, unsigned);
            kfmt_detail::emit_number(sink, ctx, v, *p == 'u' ? 10 : 16, false, width, pad);
            break;
        }
        case 'p': {
            uint64_t v = reinterpret_cast<uintptr_t>(va_arg(ap, void*));
            sink('0', ctx);
            sink('x', ctx);
            kfmt_detail::emit_number(sink, ctx, v, 16, false, 16, '0');
            break;
        }
        case 'c':
            sink(static_cast<char>(va_arg(ap, int)), ctx);
            break;
        case 's': {
            const char* s = va_arg(ap, const char*);
            if (s == nullptr) {
                s = "(null)";
            }
            int len = 0;
            while (s[len] != '\0' && (precision < 0 || len < precision)) {
                ++len;
            }
            for (int pad_len = len; pad_len < width; ++pad_len) {
                sink(' ', ctx);
            }
            for (int i = 0; i < len; ++i) {
                sink(s[i], ctx);
            }
            break;
        }
        case '%':
            sink('%', ctx);
            break;
        case '\0':
            return;              // a lone '%' at the end of the format string
        default:
            sink('%', ctx);      // unknown conversion: print it as written
            sink(*p, ctx);
            break;
        }
    }
}

// Formats into a fixed buffer, always terminated; returns the number of characters stored.
inline int ksnprintf(char* buf, int size, const char* fmt, ...)
{
    struct Ctx {
        char* buf;
        int size;
        int n;
    } c{buf, size, 0};
    KSink to_buf = [](char ch, void* p) {
        Ctx* cx = static_cast<Ctx*>(p);
        if (cx->n + 1 < cx->size) {
            cx->buf[cx->n++] = ch;
        }
    };
    va_list ap;
    va_start(ap, fmt);
    kvformat(to_buf, &c, fmt, ap);
    va_end(ap);
    if (size > 0) {
        buf[c.n] = '\0';
    }
    return c.n;
}
