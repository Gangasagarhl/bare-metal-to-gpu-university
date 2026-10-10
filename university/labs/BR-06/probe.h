// probe.h - BR-06: the interface between the arch-neutral entry probe (probe_core.cc) and
// the three arch layers (arch_x86.cc, arch_aarch64.cc, arch_riscv64.cc).
// Freestanding: no library headers at all, so the same file builds for a 32-bit x86
// Multiboot image, an AArch64 Image and a RISC-V S-mode or M-mode image.
#pragma once

using u8 = __UINT8_TYPE__;
using u32 = __UINT32_TYPE__;
using u64 = __UINT64_TYPE__;
using uptr = __UINTPTR_TYPE__;

// What the boot contract handed over, as each arch layer read it at entry.
struct EntryState {
    const char* reg_name[3];
    u64 reg_value[3];
    int nregs;
    const u8* dtb;          // devicetree blob, or nullptr where the contract passes none
    const char* privilege;  // "EL1", "S-mode", "ring 0", ...: asked of the CPU itself
    const char* how;        // how the arch layer found out
};

namespace arch {
const char* name();
void putc(char c);
[[noreturn]] void exit(bool ok);
} // namespace arch

// The core: prints the entry state; never returns.
extern "C" [[noreturn]] void probe_main(const EntryState& st);
// Called by an arch layer's exception handler: prints a report and exits.
extern "C" [[noreturn]] void probe_fatal(const char* what, u64 cause, u64 addr, u64 pc);
