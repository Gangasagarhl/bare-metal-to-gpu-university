// sv39.h - F4-28: the smallest useful Sv39 set-up: an identity map made of 1 GiB leaf entries
// in the root table. Page-table-entry bits and the satp layout after the RISC-V Instruction
// Set Manual, Volume II, "Sv39: Page-Based 39-bit Virtual-Memory System" (title only,
// pending verification). The lab's page-fault test shows QEMU walking these tables.
#pragma once
#include <cstdint>

namespace sv39 {

inline constexpr uint64_t kV = 1u << 0, kR = 1u << 1, kW = 1u << 2, kX = 1u << 3;
inline constexpr uint64_t kA = 1u << 6, kD = 1u << 7;   // set by us: no A/D-update faults
inline constexpr uint64_t kGiB = uint64_t{1} << 30;
inline constexpr uint64_t kModeSv39 = 8;               // satp.MODE value for Sv39

// Root table: 512 eight-byte entries, 4 KiB aligned. Entry i covers [i GiB, (i + 1) GiB).
// A leaf entry holds the physical page number (pa >> 12) in bits 53:10.
inline uint64_t leaf(uint64_t pa, uint64_t perms)
{
    return ((pa >> 12) << 10) | perms | kA | kD | kV;
}

inline uint64_t satp_value(const uint64_t* root)
{
    return (kModeSv39 << 60) | (reinterpret_cast<uint64_t>(root) >> 12);
}

} // namespace sv39
