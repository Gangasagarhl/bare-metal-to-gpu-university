// app.cc - F1-77 Listing 2: the program we debug. SysTick, the timer built into every
// Armv7-M core, interrupts the program; the handler counts ticks.
// SysTick register addresses and bits: Armv7-M Architecture Reference Manual, SysTick
// section (pending verification; see the unverified box in F1-77).
#include <stdint.h>

namespace {

volatile uint32_t& reg(uintptr_t address)
{
    return *reinterpret_cast<volatile uint32_t*>(address);
}

constexpr uintptr_t kUart0 = 0x40004000;
constexpr uintptr_t kSystCsr = 0xE000E010;   // control and status
constexpr uintptr_t kSystRvr = 0xE000E014;   // reload value
constexpr uintptr_t kSystCvr = 0xE000E018;   // current value

void putString(const char* s)
{
    while (*s != '\0') {
        while ((reg(kUart0 + 0x04) & 0x1u) != 0) {
        }
        reg(kUart0) = static_cast<uint8_t>(*s++);
    }
}

}  // namespace

volatile uint32_t ticks = 0;   // changed by the interrupt handler, read by main()

extern "C" void SysTick_Handler()
{
    ticks = ticks + 1;
}

int main()
{
    reg(kUart0 + 0x08) = 0x1;
    putString("F1-77: waiting for 3 SysTick interrupts\n");
    reg(kSystRvr) = 99999;                 // interrupt every 100000 processor clock cycles
    reg(kSystCvr) = 0;                     // any write clears the current value
    reg(kSystCsr) = (1u << 2) | (1u << 1) | (1u << 0);   // processor clock, interrupt, enable
    while (ticks < 3) {
        asm volatile("wfi");               // sleep until the next interrupt
    }
    putString("3 ticks seen\n");
    return 0;
}
