// serial.cc - F3-18: COM1 at I/O port 0x3F8, 115200 baud, 8 data bits, no parity, 1 stop bit.
// Register offsets and bits: PC16550D UART datasheet (pending verification, see F3-18 D4).
#include "serial.h"
#include "arch.h"

namespace {
constexpr uint16_t kCom1 = 0x3F8;
constexpr uint16_t kData = 0, kIer = 1, kFcr = 2, kLcr = 3, kMcr = 4, kLsr = 5;
constexpr uint8_t kLsrThrEmpty = 1 << 5;   // transmitter holding register empty
}

namespace serial {

void init()
{
    arch::outb(kCom1 + kIer, 0x00);   // no UART interrupts (polled output)
    arch::outb(kCom1 + kLcr, 0x80);   // DLAB = 1: the next two registers are the divisor
    arch::outb(kCom1 + 0, 0x01);      // divisor low byte: 115200 / 1
    arch::outb(kCom1 + 1, 0x00);      // divisor high byte
    arch::outb(kCom1 + kLcr, 0x03);   // DLAB = 0, 8 data bits, no parity, 1 stop bit
    arch::outb(kCom1 + kFcr, 0xC7);   // enable and clear the FIFOs
    arch::outb(kCom1 + kMcr, 0x0B);   // DTR, RTS, OUT2
}

void put(char c)
{
    while ((arch::inb(kCom1 + kLsr) & kLsrThrEmpty) == 0) {
    }
    arch::outb(kCom1 + kData, static_cast<uint8_t>(c));
}

void write(const char* s)
{
    while (*s != '\0') {
        put(*s++);
    }
}

} // namespace serial
