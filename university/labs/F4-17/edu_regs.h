// edu_regs.h - register map of PCI device 1234:11e8 as used by the DR401 driver (F4-17).
// Source of every entry: the observed specification of F4-16 (labs/F4-16/spec_observed.txt,
// evidence trace.out / annotate.out). QEMU's own documentation of this device was NOT opened in
// this build; check every line against it (and against QEMU's device model source) before use.
#pragma once
#include <stdint.h>

namespace edu_regs {

constexpr uint16_t kVendor = 0x1234;            // F4-14 devreport.out
constexpr uint16_t kDevice = 0x11E8;            // F4-14 devreport.out
constexpr uint32_t kBar0Size = 0x00100000;      // F4-14 devreport.out (size probe)

constexpr uint32_t kId = 0x00;          // RO (seen): read 0x010000ed in every run
constexpr uint32_t kCheck = 0x04;       // RW: a read after writing V returns ~V
constexpr uint32_t kCompute = 0x08;     // RW: write N; when STATUS bit 0 clears, reads N!
constexpr uint32_t kStatus = 0x20;      // R: bit 0 = computation in progress
constexpr uint32_t kIrqStatus = 0x24;   // R: value raised and not yet acknowledged
constexpr uint32_t kIrqRaise = 0x60;    // W: sets IRQ_STATUS to the value written
constexpr uint32_t kIrqAck = 0x64;      // W: clears IRQ_STATUS (value written = value raised)

constexpr uint32_t kStatusBusy = 1u << 0;
constexpr uint32_t kIdSeen = 0x010000ED;    // the only value observed; meaning unknown

} // namespace edu_regs
