// apic.h - F3-24: legacy 8259 PICs (switched off), the local APIC (xAPIC or x2APIC) and the
// IOAPIC, plus the routing layer that turns an ISA IRQ into a vector on this CPU.
#pragma once
#include <cstdint>
#include "interrupts.h"
#include "madt.h"

namespace apic {

void disable_8259();                        // remap to 0x20..0x2F, then mask every line
bool init_local(uint64_t lapic_phys);       // x2APIC when the CPU offers it, else xAPIC
bool x2apic_mode();
uint32_t local_id();
void eoi();
uint32_t read_lapic(uint32_t offset);       // xAPIC register offset (0x20 = ID ...); x2APIC: MSR 0x800 + offset/16
void write_lapic(uint32_t offset, uint32_t value);

bool init_ioapic(const madt::Info& m);      // maps the first IOAPIC, masks all its pins
// Routes ISA 'irq' to a newly allocated vector on this CPU, installs 'h', unmasks the pin.
// Returns the vector, or 0 on failure.
uint8_t route_isa_irq(const madt::Info& m, uint8_t irq, IrqHandler h);
void print_redirection(uint32_t gsi);

uint64_t map_mmio(uint64_t phys, uint64_t size);   // uncached kernel mapping of device registers
uint64_t spurious_count();

} // namespace apic
