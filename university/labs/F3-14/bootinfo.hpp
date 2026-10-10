// bootinfo.hpp - the OS301 boot protocol: what our loader (F3-15) hands to our kernel.
// This is our own design (milestone A3), not a standard. The rules of the protocol:
//   * the kernel is an ELF64 x86-64 executable linked in the higher half (0xffffffff80000000 up);
//   * the loader maps every PT_LOAD segment with the permissions of its flags (R, W, X);
//   * all physical memory is also mapped at kDirectMapBase + physical address ("direct map");
//   * the kernel is entered with interrupts off, boot services exited, RSP pointing at a
//     loader-provided stack, and RDI holding the virtual address of a BootInfo (System V ABI);
//   * BootInfo starts with a magic number, a version and its own size, so a newer kernel can
//     detect an older loader and fields can only ever be added at the end.
#pragma once
#include <stdint.h>

namespace os301 {

constexpr uint64_t kBootMagic = 0x3130335342534f00ull;  // bytes: 00 'O' 'S' 'B' 'S' '3' '0' '1'
constexpr uint32_t kBootInfoVersion = 1;
constexpr uint64_t kDirectMapBase = 0xffff800000000000ull;

struct MemoryRange {                // a copy of one UEFI memory descriptor's useful fields
    uint32_t type;                  // UEFI memory type number (7 = conventional, ...)
    uint32_t reserved;
    uint64_t physical_start;
    uint64_t pages;                 // 4 KiB pages
    uint64_t attribute;
};

struct BootInfo {
    uint64_t magic;                 // kBootMagic
    uint32_t version;               // kBootInfoVersion
    uint32_t size;                  // sizeof(BootInfo) as the loader knew it
    // the final memory map, taken just before ExitBootServices
    uint64_t memory_map;            // virtual address (direct map) of MemoryRange[memory_map_count]
    uint64_t memory_map_count;
    // the framebuffer set up by the firmware's Graphics Output Protocol
    uint64_t fb_physical;
    uint64_t fb_size;
    uint32_t fb_width, fb_height;   // pixels
    uint32_t fb_pitch;              // bytes per row
    uint32_t fb_format;             // UEFI pixel format number
    // firmware tables (physical addresses; 0 = not found)
    uint64_t rsdp_physical;
    uint64_t smbios3_physical;
    // where the kernel is
    uint64_t kernel_physical;
    uint64_t kernel_virtual;
    uint64_t kernel_size;
    uint64_t direct_map_base;       // kDirectMapBase
    char command_line[128];         // from \cmdline.txt on the ESP, may be empty
    uint64_t initrd_physical;       // optional initial RAM disk (0 = none)
    uint64_t initrd_size;
    // boot timeline: time-stamp counter readings taken by the loader (an FPDT-like record)
    uint64_t tsc_loader_entry;
    uint64_t tsc_kernel_file_read;
    uint64_t tsc_before_exit;       // just before the final GetMemoryMap + ExitBootServices
    uint64_t tsc_after_exit;
    uint32_t exit_attempts;         // how many ExitBootServices calls were needed
    uint32_t pad;
};

}  // namespace os301
