// irq.h - DR301 F4-03: IDT, legacy 8259 PIC and interrupt handler registration.
// The lab kernel keeps the two 8259 PICs of the PC for brevity; a B7 kernel routes the
// same lines through the IOAPIC and acknowledges at the local APIC instead (see F4-03).
#pragma once
#include <stdint.h>

struct IrqFrame {                // pushed by isr.S (edi first in memory) and by the CPU
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t vector, error_code;
    uint32_t eip, cs, eflags;
};

using IrqHandler = void (*)();

namespace irq {
void init();                                 // IDT for vectors 0-47, PICs remapped to 32-47
void set_handler(uint8_t line, IrqHandler h);// device line 0-15
void unmask(uint8_t line);
void mask(uint8_t line);
uint32_t count(uint8_t line);                // interrupts delivered on this line so far
inline void enable() { asm volatile("sti"); }
inline void disable() { asm volatile("cli"); }
inline void wait() { asm volatile("sti; hlt"); }   // sleep until the next interrupt
}

// A 1 kHz tick from the PIT (channel 0): the lab kernel's monotonic clock.
namespace timer {
void init();
uint64_t ms();                               // milliseconds since timer::init()
void sleep_ms(uint32_t n);
}
