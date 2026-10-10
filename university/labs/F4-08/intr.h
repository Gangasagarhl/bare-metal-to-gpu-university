// intr.h - DR302 F4-08: the interrupt layer every DR302 driver uses.
// A 256-entry IDT, the local APIC (acknowledge), the IOAPIC (legacy lines, routed from the
// ACPI MADT) and a 1 kHz tick. The 8259 PICs of DR301 are remapped and fully masked.
#pragma once
#include <stdint.h>

namespace intr {
using Handler = void (*)(uint8_t vector);

// Reads the MADT, masks the PICs, enables the local APIC, starts the 1 kHz tick
// (PIT -> IOAPIC -> vector 0x20) and enables interrupts.
void init();
uint8_t alloc_vector();                       // a free vector >= 0x40 for a device
void set_handler(uint8_t vector, Handler h);
uint32_t count(uint8_t vector);               // interrupts delivered on this vector
uint32_t lapic_id();                          // this CPU's local APIC ID (MSI destination)
uintptr_t lapic_base();
// Route ISA IRQ 'irq' (after MADT overrides) or a GSI to 'vector'.
// level/active_low come from the MADT override or the caller (PCI lines are level, low).
uint32_t isa_to_gsi(uint8_t irq, bool* level, bool* active_low);
void route_gsi(uint32_t gsi, uint8_t vector, bool level, bool active_low);
void mask_gsi(uint32_t gsi, bool masked);
void print_routing();                         // MADT summary and IOAPIC redirection entries

inline void enable() { asm volatile("sti"); }
inline void disable() { asm volatile("cli"); }
inline void wait() { asm volatile("sti; hlt"); }
}

namespace clock {
uint64_t ms();                                // milliseconds since intr::init()
void sleep_ms(uint32_t n);
}
