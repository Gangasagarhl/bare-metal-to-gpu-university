// multiboot.h - F3-18: the parts of the Multiboot (version 1) information structure we read.
// Layout from the Multiboot Specification (version 1; pending verification, F3-18 D2);
// the lab's run shows QEMU filling these fields.
#pragma once
#include <cstdint>

namespace mb {

inline constexpr uint32_t kBootMagic = 0x2BADB002;   // in EAX at entry
inline constexpr uint32_t kFlagMem = 1u << 0;        // mem_lower / mem_upper valid
inline constexpr uint32_t kFlagCmdline = 1u << 2;    // cmdline valid
inline constexpr uint32_t kFlagMmap = 1u << 6;       // mmap_length / mmap_addr valid

struct Info {
    uint32_t flags;
    uint32_t mem_lower;       // KiB below 1 MiB
    uint32_t mem_upper;       // KiB above 1 MiB
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed));

struct MmapEntry {            // 'size' does not count itself: next = this + size + 4
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;            // 1 = available RAM; other values are reserved kinds
} __attribute__((packed));

} // namespace mb
