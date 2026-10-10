// main.cc - first firmware for the emulated lab microcontroller (QEMU mps2-an385).
// Prints a banner on UART0 and proves that start-up code did its job.
// UART register offsets: Arm CMSDK APB UART, as modelled by QEMU 8.2 (see the
// unverified box in F1-73: check the CMSDK technical reference manual).
#include <stdint.h>

namespace {

constexpr uintptr_t kUart0Base = 0x40004000;  // "uart" region in QEMU's memory tree

struct CmsdkUart {
    volatile uint32_t data;    // 0x00 write a byte to send it
    volatile uint32_t state;   // 0x04 bit 0 = transmit buffer full
    volatile uint32_t ctrl;    // 0x08 bit 0 = transmit enable
};

CmsdkUart& uart0()
{
    return *reinterpret_cast<CmsdkUart*>(kUart0Base);
}

void putChar(char c)
{
    while ((uart0().state & 0x1u) != 0) {
        // wait until the transmit buffer has room
    }
    uart0().data = static_cast<uint8_t>(c);
}

void putString(const char* s)
{
    while (*s != '\0') {
        putChar(*s++);
    }
}

void putHex(uint32_t v)
{
    putString("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        putChar("0123456789abcdef"[(v >> shift) & 0xFu]);
    }
}

}  // namespace

extern "C" uint32_t __stack_top;
extern "C" const uint32_t vector_table[];

uint32_t boot_count = 7;    // initialised: lives in .data, starts in code memory
uint32_t ticks;             // zero-initialised: lives in .bss

int main()
{
    uart0().ctrl = 0x1;  // enable the transmitter
    putString("HW303 lab MCU: hello from bare metal\n");
    putString("vector table at ");
    putHex(reinterpret_cast<uintptr_t>(vector_table));
    putString(", initial SP ");
    putHex(vector_table[0]);
    putString(", reset handler ");
    putHex(vector_table[1]);
    putString("\nboot_count (expect 7) = ");
    putHex(boot_count);
    putString("\nticks (expect 0) = ");
    putHex(ticks);
    putString("\nstack top symbol = ");
    putHex(reinterpret_cast<uintptr_t>(&__stack_top));
    putString("\nmain() done; the reset handler now waits forever\n");
    return 0;
}
