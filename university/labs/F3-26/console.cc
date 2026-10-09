// console.cc - COM1 serial console (16550 UART registers, PC16550D datasheet; title
// only), kprintf, panic and the QEMU test-exit device.
#include "cpu.h"
#include "sync.h"

namespace k {

namespace {
constexpr uint16_t kCom1 = 0x3F8;
Spinlock console_lock("console", false);   // unchecked: the lock checker prints through it
volatile bool panicking = false;
volatile bool quiet = false;

void uart_putc(char ch)
{
    while ((inb(kCom1 + 5) & 0x20) == 0) {   // line status: transmit holding register empty
        cpu_relax();
    }
    outb(kCom1, uint8_t(ch));
}

struct Buf {
    char data[320];
    size_t n = 0;
    void put(char ch)
    {
        if (n < sizeof(data)) {
            data[n++] = ch;
        }
    }
};

void put_num(Buf& b, uint64_t v, unsigned base, bool neg, int width, char pad)
{
    char tmp[24];
    int i = 0;
    do {
        tmp[i++] = "0123456789abcdef"[v % base];
        v /= base;
    } while (v != 0);
    if (neg) {
        tmp[i++] = '-';
    }
    while (i < width--) {
        b.put(pad);
    }
    while (i > 0) {
        b.put(tmp[--i]);
    }
}
}  // namespace

void console_init()
{
    outb(kCom1 + 1, 0x00);   // no UART interrupts
    outb(kCom1 + 3, 0x80);   // DLAB on: next two writes set the baud divisor
    outb(kCom1 + 0, 0x01);   // divisor 1
    outb(kCom1 + 1, 0x00);
    outb(kCom1 + 3, 0x03);   // 8 data bits, no parity, 1 stop bit
    outb(kCom1 + 2, 0xC7);   // FIFO on and cleared
    outb(kCom1 + 4, 0x03);   // DTR, RTS
}

void console_set_quiet(bool q) { quiet = q; }
void console_panic_mode()
{
    panicking = true;
    quiet = false;
}
bool console_quiet() { return quiet; }

void console_write_raw(const char* s, size_t n)
{
    uint64_t f = 0;
    if (!panicking) {
        f = console_lock.lock_irqsave();
    }
    for (size_t i = 0; i < n; ++i) {
        uart_putc(s[i]);
    }
    if (!panicking) {
        console_lock.unlock_irqrestore(f);
    }
}

void kvprintf(const char* fmt, va_list ap)
{
    Buf b;
    for (const char* p = fmt; *p != '\0'; ++p) {
        if (*p != '%') {
            b.put(*p);
            continue;
        }
        ++p;
        char pad = ' ';
        int width = 0;
        if (*p == '0') {
            pad = '0';
            ++p;
        }
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p++ - '0');
        }
        int longs = 0;
        while (*p == 'l' || *p == 'z') {
            ++longs;
            ++p;
        }
        switch (*p) {
        case 'd': {
            int64_t v = longs ? va_arg(ap, int64_t) : va_arg(ap, int);
            put_num(b, v < 0 ? uint64_t(-v) : uint64_t(v), 10, v < 0, width, pad);
            break;
        }
        case 'u':
            put_num(b, longs ? va_arg(ap, uint64_t) : va_arg(ap, unsigned), 10, false, width, pad);
            break;
        case 'x':
            put_num(b, longs ? va_arg(ap, uint64_t) : va_arg(ap, unsigned), 16, false, width, pad);
            break;
        case 'p':
            b.put('0');
            b.put('x');
            put_num(b, reinterpret_cast<uint64_t>(va_arg(ap, void*)), 16, false, width, pad);
            break;
        case 's': {
            const char* s = va_arg(ap, const char*);
            int len = 0;
            for (const char* q = s ? s : "(null)"; *q != '\0'; ++q, ++len) {
                b.put(*q);
            }
            while (len++ < width) {
                b.put(' ');
            }
            break;
        }
        case 'c':
            b.put(char(va_arg(ap, int)));
            break;
        default:
            b.put('%');
            b.put(*p);
        }
    }
    console_write_raw(b.data, b.n);
}

void kprintf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
}

void qemu_exit(int code)
{
    // isa-debug-exit device at I/O port 0xf4 (configured on the QEMU command line);
    // QEMU exits with status (code << 1) | 1.
    outb(0xf4, uint8_t(code));
    for (;;) {
        asm volatile("cli; hlt");
    }
}

void panic(const char* fmt, ...)
{
    irq_disable();
    panicking = true;
    quiet = false;
    kprintf("PANIC on cpu%d: ", this_cpu().id);
    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
    kprintf("\n");
    qemu_exit(1);   // QEMU exit status 3 = failure
}

void str_copy(char* dst, const char* src, size_t cap)
{
    size_t i = 0;
    for (; i + 1 < cap && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

bool str_eq(const char* a, const char* b)
{
    while (*a != '\0' && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}

size_t str_len(const char* s)
{
    size_t n = 0;
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

}  // namespace k

// rep stosb / rep movsb: a plain C++ loop here could be turned back into a call to
// memset or memcpy by the optimiser, which would recurse forever.
extern "C" void* memset(void* d, int c, size_t n)
{
    void* p = d;
    asm volatile("rep stosb" : "+D"(p), "+c"(n) : "a"(c) : "memory");
    return d;
}

extern "C" void* memcpy(void* d, const void* s, size_t n)
{
    void* p = d;
    asm volatile("rep movsb" : "+D"(p), "+S"(s), "+c"(n) : : "memory");
    return d;
}

extern "C" void* memmove(void* d, const void* s, size_t n)
{
    auto* dp = static_cast<unsigned char*>(d);
    auto* sp = static_cast<const unsigned char*>(s);
    if (dp < sp) {
        for (size_t i = 0; i < n; ++i) {
            dp[i] = sp[i];
        }
    } else {
        for (size_t i = n; i > 0; --i) {
            dp[i - 1] = sp[i - 1];
        }
    }
    return d;
}

extern "C" int memcmp(const void* a, const void* b, size_t n)
{
    auto* x = static_cast<const unsigned char*>(a);
    auto* y = static_cast<const unsigned char*>(b);
    for (size_t i = 0; i < n; ++i) {
        if (x[i] != y[i]) {
            return x[i] < y[i] ? -1 : 1;
        }
    }
    return 0;
}
