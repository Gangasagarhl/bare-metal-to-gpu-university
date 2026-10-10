// BR-05 lab: copied unchanged from university/labs/F3-40/board.h (only this line added).
// board.h - OS305 board support for the lab microcontroller (QEMU mps2-an385, Cortex-M3).
// Addresses: QEMU's memory tree (mtree.out). Register layouts: Arm CMSDK APB UART and the
// MPS2 FPGA I/O block as modelled by QEMU 8.2, and the Armv7-M SysTick (pending
// verification against the Arm documents named in the chapter's unverified box).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace board {

// A memory-mapped register: every access really goes to the bus (volatile).
inline volatile uint32_t& reg(uintptr_t address)
{
    return *reinterpret_cast<volatile uint32_t*>(address);
}

// --- UART0 (CMSDK APB UART) --------------------------------------------------------------
struct UartRegs {
    volatile uint32_t data;      // 0x00 write: send a byte; read: received byte
    volatile uint32_t state;     // 0x04 bit 0: TX buffer full, bit 1: RX buffer full
    volatile uint32_t ctrl;      // 0x08 bit 0: TX enable, bit 1: RX enable, bits 2/3: TX/RX irq
    volatile uint32_t intstatus; // 0x0C read: pending interrupts; write 1 to clear
    volatile uint32_t bauddiv;   // 0x10 baud-rate divider
};
static_assert(offsetof(UartRegs, bauddiv) == 0x10, "UART register layout");
inline constexpr uintptr_t kUart0Base = 0x40004000;
inline UartRegs& uart0() { return *reinterpret_cast<UartRegs*>(kUart0Base); }

inline void uartInit()
{
    uart0().bauddiv = 16;        // the model needs a divider of at least 16 to accept data
    uart0().ctrl = 0x1;          // transmitter on
}

inline void putChar(char c)
{
    while ((uart0().state & 0x1u) != 0) {
        // wait while the transmit buffer is full
    }
    uart0().data = static_cast<uint8_t>(c);
}

inline void print(const char* s)
{
    while (*s != '\0') {
        putChar(*s++);
    }
}

inline void printDec(uint32_t v)
{
    char buf[11];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v != 0);
    while (n > 0) {
        putChar(buf[--n]);
    }
}

inline void printHex(uint32_t v)
{
    print("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        putChar("0123456789abcdef"[(v >> shift) & 0xFu]);
    }
}

// --- LEDs (MPS2 FPGA I/O block, LED register at offset 0) ---------------------------------
inline constexpr uintptr_t kFpgaIoBase = 0x40028000;
inline void setLeds(uint32_t bits) { reg(kFpgaIoBase + 0x00) = bits; }

// --- SysTick, part of every Armv7-M core ----------------------------------------------------
inline constexpr uintptr_t kSystCsr = 0xE000E010;   // control and status
inline constexpr uintptr_t kSystRvr = 0xE000E014;   // reload value
inline constexpr uintptr_t kSystCvr = 0xE000E018;   // current value

// Busy-wait for n periods of `reload + 1` processor clocks, polling COUNTFLAG (bit 16).
inline void delayPeriods(uint32_t reload, uint32_t n)
{
    reg(kSystRvr) = reload;
    reg(kSystCvr) = 0;
    reg(kSystCsr) = (1u << 2) | (1u << 0);        // processor clock, enable, no interrupt
    for (uint32_t i = 0; i < n; ++i) {
        while ((reg(kSystCsr) & (1u << 16)) == 0) {
        }
    }
    reg(kSystCsr) = 0;
}

// --- Leaving the emulator: Arm semihosting SYS_EXIT (only with -semihosting) ---------------
[[noreturn]] inline void exitEmulator(bool success)
{
    // r0 = operation 0x18 (SYS_EXIT); r1 = reason: 0x20026 "application exit" (success),
    // 0x20023 "run-time error" (failure).
    register uint32_t r0 asm("r0") = 0x18;
    register uint32_t r1 asm("r1") = success ? 0x20026u : 0x20023u;
    asm volatile("bkpt #0xab" : : "r"(r0), "r"(r1) : "memory");
    for (;;) {
    }
}

}  // namespace board
