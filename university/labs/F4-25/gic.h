// gic.h - F4-25: a GICv3 driver: distributor, one redistributor per CPU, and the CPU
// interface through system registers. Register offsets and bits after the "Arm Generic
// Interrupt Controller Architecture Specification, GIC architecture version 3 and version 4"
// (title only, pending verification); the lab's timer and UART interrupts exercise them.
#pragma once
#include <cstdint>
#include "fdt.h"

namespace gic {

enum class Version { None, V2, V3 };

// Finds the interrupt controller in the devicetree. For GICv3 it also records the
// distributor and redistributor-region bases (checklist row 4).
Version probe(const fdt::Blob& dt);
const char* compatible();            // the compatible string that was found

void init_distributor();             // once, on the boot CPU
bool init_cpu();                     // on every CPU: its redistributor and CPU interface
void enable_ppi(uint32_t intid, uint8_t priority);                 // INTIDs 16-31, per CPU
void enable_spi(uint32_t intid, uint8_t priority, bool level);     // INTIDs 32-1019, shared
void send_sgi(uint32_t intid, uint64_t target_mpidr);              // INTIDs 0-15 (F4-26)
uint32_t ack();                      // ICC_IAR1_EL1: INTID of the highest pending interrupt
void eoi(uint32_t intid);            // ICC_EOIR1_EL1: priority drop and deactivate
void dump();                         // a few state registers, for debugging

inline constexpr uint32_t kSpurious = 1023;

} // namespace gic
