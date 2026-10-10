// arch.h - F4-24: the AArch64 arch layer's small inline helpers. It offers the same names
// as F3-18's x86-64 arch.h where the arch-neutral code needs them (cli, halt_forever,
// qemu_exit, kExitPass, kExitFail), so F3-18's panic.cc compiles unchanged on AArch64.
// System-register names after the Arm Architecture Reference Manual (title only, pending
// verification); the assembler accepting each name is checked by the build.
#pragma once
#include <cstdint>

// mrs / msr wrappers. The register name must be a literal, so these are macros; the lambda
// keeps them ordinary C++ expressions (no compiler extensions).
#define READ_SYSREG(reg)                                     \
    [] {                                                     \
        uint64_t v_;                                         \
        asm volatile("mrs %0, " #reg : "=r"(v_));            \
        return v_;                                           \
    }()
#define WRITE_SYSREG(reg, val)                                                   \
    do {                                                                         \
        uint64_t v_ = (val);                                                     \
        asm volatile("msr " #reg ", %0" : : "r"(v_) : "memory");                 \
    } while (0)

namespace arch {

inline constexpr const char* kName = "aarch64";
inline constexpr uint64_t kPageSize = 4096;     // the granule this port chose (F4-25)

inline void cli() { asm volatile("msr daifset, #0x2" : : : "memory"); }   // mask IRQ
inline void sti() { asm volatile("msr daifclr, #0x2" : : : "memory"); }   // unmask IRQ
inline void wfi() { asm volatile("wfi" : : : "memory"); }
inline void cpu_relax() { asm volatile("yield" : : : "memory"); }
// Lock waiting: sleep until an event (WFE); the unlocking CPU sends one (SEV) after its
// release store is visible (DSB). Under QEMU's TCG a WFE also gives the host CPU away.
inline void spin_wait() { asm volatile("wfe" : : : "memory"); }
inline void spin_wake() { asm volatile("dsb ishst\n\tsev" : : : "memory"); }
inline void isb() { asm volatile("isb" : : : "memory"); }
inline void dsb_sy() { asm volatile("dsb sy" : : : "memory"); }

inline uint64_t current_el() { return (READ_SYSREG(CurrentEL) >> 2) & 3; }

[[noreturn]] inline void halt_forever()
{
    for (;;) {
        asm volatile("msr daifset, #0xf\n\twfi" : : : "memory");
    }
}

// Same names and values as F3-18 so arch-neutral code does not change. On AArch64 there is
// no isa-debug-exit device: qemu_exit prints the status and asks the firmware to power off
// through PSCI SYSTEM_OFF (psci.cc), so QEMU itself exits with status 0.
inline constexpr uint8_t kExitPass = 0x10;
inline constexpr uint8_t kExitFail = 0x11;
[[noreturn]] void qemu_exit(uint8_t code);

} // namespace arch
