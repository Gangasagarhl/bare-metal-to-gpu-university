// k4.cc - DR401 lab kernel base (see k4.h).
// The 16550 register names and bits come from the Linux UAPI header <linux/serial_reg.h>
// (installed with linux-libc-dev in this build); COM1 at I/O port 0x3F8 is the PC convention.
#include "k4.h"
#include <linux/serial_reg.h>

namespace k4 {

static constexpr uint16_t kCom1 = 0x3F8;
static constexpr uint16_t kDebugExit = 0xF4;   // -device isa-debug-exit,iobase=0xf4

void console_init()
{
    outb(kCom1 + UART_IER, 0x00);                       // no UART interrupts in this kernel
    outb(kCom1 + UART_LCR, UART_LCR_DLAB);              // divisor latch on
    outb(kCom1 + UART_DLL, 0x01);                       // divisor 1 (fastest rate)
    outb(kCom1 + UART_DLM, 0x00);
    outb(kCom1 + UART_LCR, UART_LCR_WLEN8);             // 8N1, divisor latch off
    outb(kCom1 + UART_FCR, UART_FCR_ENABLE_FIFO);
    outb(kCom1 + UART_MCR, UART_MCR_DTR | UART_MCR_RTS);
}

void putc(char c)
{
    if (c == '\n') putc('\r');
    for (int i = 0; i < 100000 && !(inb(kCom1 + UART_LSR) & UART_LSR_THRE); ++i) {
    }
    outb(kCom1 + UART_TX, static_cast<uint8_t>(c));
}

void puts(const char* s)
{
    while (*s) putc(*s++);
}

void line(const char* s)
{
    puts(s);
    putc('\n');
}

void hex(uint64_t v, int digits)
{
    for (int i = digits - 1; i >= 0; --i) putc("0123456789abcdef"[(v >> (4 * i)) & 0xF]);
}

uint64_t udiv64(uint64_t n, uint64_t d)
{
    uint64_t q = 0, r = 0;
    for (int i = 63; i >= 0; --i) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) { r -= d; q |= (uint64_t{1} << i); }
    }
    return q;
}

void dec(uint64_t v)
{
    char buf[21];
    int n = 0;
    do {
        uint64_t q = udiv64(v, 10);
        buf[n++] = static_cast<char>('0' + (v - q * 10));
        v = q;
    } while (v);
    while (n) putc(buf[--n]);
}

void exit_qemu(uint8_t code)
{
    outb(kDebugExit, code);
    for (;;) asm volatile("cli; hlt");
}

void panic(const char* why)
{
    puts("PANIC: ");
    line(why);
    exit_qemu(0x7f);
}

} // namespace k4

// The compiler may emit calls to these even in freestanding code (structure copies).
extern "C" void* memset(void* d, int c, size_t n)
{
    auto* p = static_cast<unsigned char*>(d);
    while (n--) *p++ = static_cast<unsigned char>(c);
    return d;
}
extern "C" void* memcpy(void* d, const void* s, size_t n)
{
    auto* p = static_cast<unsigned char*>(d);
    auto* q = static_cast<const unsigned char*>(s);
    while (n--) *p++ = *q++;
    return d;
}
