// mmu.h - F4-25: VMSAv8-64 translation with a 4 KiB granule and a 39-bit virtual address
// space (walk starts at level 1). Descriptor bits, MAIR encodings and TCR fields after the
// Arm Architecture Reference Manual, chapter "The AArch64 Virtual Memory System
// Architecture" (title only, pending verification). The lab's fault tests show QEMU
// walking the tables this code builds.
#pragma once
#include <cstdint>

namespace mmu {

// Attribute indexes into MAIR_EL1 (mmu.cc programs the matching bytes).
inline constexpr uint64_t kAttrDevice = 0;   // Device-nGnRnE: MMIO, no gathering or reordering
inline constexpr uint64_t kAttrNormal = 1;   // Normal, write-back cacheable: RAM

// Descriptor bits.
inline constexpr uint64_t kValid = 1u << 0;
inline constexpr uint64_t kTableOrPage = 1u << 1;   // level 1/2: table; level 3: page
inline constexpr uint64_t kApReadOnly = 1u << 7;    // AP[2] = 1: read-only (EL1 only: AP[1] = 0)
inline constexpr uint64_t kInnerShareable = 3u << 8;
inline constexpr uint64_t kAccessFlag = 1u << 10;   // set, or the first access faults
inline constexpr uint64_t kPXN = uint64_t{1} << 53; // never executable at EL1
inline constexpr uint64_t kUXN = uint64_t{1} << 54; // never executable at EL0
inline uint64_t attr(uint64_t index) { return index << 2; }

// Builds the identity map (GiB 0: devices; GiB 1: RAM with per-section permissions for the
// kernel image), programs MAIR/TCR/TTBR0 and turns on the MMU and caches.
void init(uint64_t ram_base, uint64_t ram_size);
// Programs this CPU's MAIR/TCR/TTBR0/SCTLR for the tables init() built (secondary CPUs, F4-26).
void enable_this_cpu();

// Walks the tables in software, the way the MMU does, and prints every step.
void explain(uint64_t va);

} // namespace mmu
