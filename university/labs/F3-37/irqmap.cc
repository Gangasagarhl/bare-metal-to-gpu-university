// irqmap.cc - F3-37 Listing 2: which NVIC line does each peripheral of the emulated board
// use? Enable every IRQ line, make timer 0 and the UART raise their interrupts, and let one
// shared handler report the active exception number (IPSR). Measured, not assumed.
#include "board.h"

namespace {

volatile uint32_t seen[16];   // seen[n] = how many times IRQ n arrived

uint32_t ipsr()
{
    uint32_t v;
    asm volatile("mrs %0, ipsr" : "=r"(v));
    return v;
}

void anyIrq()
{
    const uint32_t irq = ipsr() - 16;      // exception number 16 is IRQ 0
    if (irq < 16) {
        seen[irq] = seen[irq] + 1;
    }
    board::reg(0x40000000 + 0x0C) = 1;     // timer 0 INTCLEAR
    board::reg(0xE000E180) = 1u << irq;    // NVIC ICER0: disable this line (stop repeats)
}

}  // namespace

#define OS305_IRQ(n) extern "C" void Irq##n##_Handler() { anyIrq(); }
OS305_IRQ(0) OS305_IRQ(1) OS305_IRQ(2) OS305_IRQ(3) OS305_IRQ(4) OS305_IRQ(5)
OS305_IRQ(6) OS305_IRQ(7) OS305_IRQ(8) OS305_IRQ(9) OS305_IRQ(10) OS305_IRQ(11)
OS305_IRQ(12) OS305_IRQ(13) OS305_IRQ(14) OS305_IRQ(15)

int main()
{
    board::uartInit();
    board::print("F3-37 irqmap: which IRQ line does each peripheral raise?\n");
    board::reg(0xE000E100) = 0xFFFF;                   // NVIC ISER0: enable IRQ 0-15

    board::print("timer 0 ... ");
    board::reg(0x40000000 + 0x08) = 1000;              // RELOAD
    board::reg(0x40000000 + 0x04) = 1000;              // VALUE
    board::reg(0x40000000 + 0x00) = (1u << 3) | 1u;    // CTRL: interrupt enable, enable
    board::delayPeriods(9999, 5);
    board::reg(0x40000000 + 0x00) = 0;                 // stop the timer
    board::reg(0x40000000 + 0x0C) = 1;                 // and clear its last flag
    for (uint32_t n = 0; n < 16; ++n) {
        if (seen[n] != 0) { board::print("IRQ "); board::printDec(n); board::print(" "); seen[n] = 0; }
    }
    board::print("\nUART 0 receive ... ");
    board::reg(0xE000E280) = 0xFFFF;                   // NVIC ICPR0: forget pending IRQ 0-15
    board::reg(0xE000E100) = 0xFFFF;                   // and enable them again
    board::uart0().ctrl = 0x1 | 0x2 | (1u << 3);      // TX, RX, RX interrupt enable
    board::delayPeriods(9999, 50);                     // a byte from stdin arrives meanwhile
    for (uint32_t n = 0; n < 16; ++n) {
        if (seen[n] != 0) { board::print("IRQ "); board::printDec(n); board::print(" "); }
    }
    board::print("\n");
    board::exitEmulator(true);
}
