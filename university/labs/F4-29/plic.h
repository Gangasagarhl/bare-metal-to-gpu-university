// plic.h - F4-29: the RISC-V Platform-Level Interrupt Controller. Register layout after the
// "RISC-V Platform-Level Interrupt Controller Specification" (title only, pending
// verification). Which PLIC context belongs to which hart and privilege mode is NOT fixed:
// it is read from the devicetree's interrupts-extended property.
#pragma once
#include <cstdint>
#include "fdt.h"

namespace plic {

// Finds the PLIC and the S-mode external-interrupt context of every hart. False if absent.
bool probe(const fdt::Blob& dt);
int s_context(uint64_t hartid);                 // -1 if the hart has no S-mode context
void init_hart(uint64_t hartid);                // threshold 0 for this hart's S context
void enable(uint64_t hartid, uint32_t source, uint32_t priority);
uint32_t claim(uint64_t hartid);                // 0 = nothing pending
void complete(uint64_t hartid, uint32_t source);
void dump(uint64_t hartid, uint32_t source);    // pending, enable, threshold, for debugging

} // namespace plic
