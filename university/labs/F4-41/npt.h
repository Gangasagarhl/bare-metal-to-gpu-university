// npt.h - guest physical memory through nested page tables (AMD NPT; Intel calls the
// same idea EPT), and loading a kernel ELF file into it.
#pragma once
#include <stdint.h>
#include "hv.h"

constexpr uint64_t GUEST_RAM_SIZE = 32ull << 20;    // guest-physical 0 .. 32 MiB

uint8_t* guest_ram();                                // host address of guest-physical 0
uint64_t npt_build();                                // returns the nested CR3 (host PA)

struct GuestImage {
    uint64_t long_mode = 0;      // 64-bit entry point (symbol long_mode)
    uint64_t boot_magic = 0;     // where the entry code expects the Multiboot magic ...
    uint64_t boot_info = 0;      // ... and the Multiboot information address
    uint64_t gdt64_ptr = 0;      // the kernel's own GDT descriptor (limit, base)
    uint64_t entry32 = 0;        // the ELF entry point: the 32-bit Multiboot entry _start
};
// Copies the ELF64 file's loadable segments to their physical addresses in guest RAM and
// finds the four symbols above in its symbol table. Returns false if anything is missing.
bool load_guest_elf(const uint8_t* elf, uint64_t size, GuestImage& img);
// Plays the boot loader: Multiboot information with the command line, page tables in
// guest memory, and a 64-bit entry state in the VMCB (see the chapter for why 64-bit).
void vcpu_init_guest_kernel(Vcpu& v, Vmcb* vmcb, const GuestImage& img, const char* cmdline,
                            uint64_t ncr3);
