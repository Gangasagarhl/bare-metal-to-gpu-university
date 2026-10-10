// arch.h - F4-28: the RISC-V (RV64, S-mode) arch layer's small inline helpers. Same names as
// F3-18's x86-64 arch.h and F4-24's AArch64 arch.h where arch-neutral code needs them.
// CSR names after the RISC-V Instruction Set Manual, Volume II (title only, pending
// verification); the assembler accepting each name is checked by the build.
#pragma once
#include <cstdint>

#define CSR_READ(csr)                                        \
    [] {                                                     \
        uint64_t v_;                                         \
        asm volatile("csrr %0, " #csr : "=r"(v_));           \
        return v_;                                           \
    }()
#define CSR_WRITE(csr, val)                                                      \
    do {                                                                         \
        uint64_t v_ = (val);                                                     \
        asm volatile("csrw " #csr ", %0" : : "r"(v_) : "memory");                \
    } while (0)
#define CSR_SET(csr, bits)                                                       \
    do {                                                                         \
        uint64_t v_ = (bits);                                                    \
        asm volatile("csrs " #csr ", %0" : : "r"(v_) : "memory");                \
    } while (0)
#define CSR_CLEAR(csr, bits)                                                     \
    do {                                                                         \
        uint64_t v_ = (bits);                                                    \
        asm volatile("csrc " #csr ", %0" : : "r"(v_) : "memory");                \
    } while (0)

namespace arch {

inline constexpr const char* kName = "riscv64";
inline constexpr uint64_t kPageSize = 4096;      // Sv39 base page

inline constexpr uint64_t kSstatusSIE = 1u << 1;
inline void cli() { CSR_CLEAR(sstatus, kSstatusSIE); }
inline void sti() { CSR_SET(sstatus, kSstatusSIE); }
inline void wfi() { asm volatile("wfi" : : : "memory"); }
inline void cpu_relax() { asm volatile("nop" : : : "memory"); }
inline void spin_wait() { cpu_relax(); }   // no wait-for-event instruction in RV64IMAC
inline void spin_wake() {}
inline void sfence_vma() { asm volatile("sfence.vma zero, zero" : : : "memory"); }

[[noreturn]] inline void halt_forever()
{
    for (;;) {
        CSR_CLEAR(sstatus, kSstatusSIE);
        asm volatile("wfi" : : : "memory");
    }
}

// Same names and values as F3-18. qemu_exit prints the status and asks the SBI firmware to
// shut the system down (SBI System Reset extension), so QEMU exits with status 0.
inline constexpr uint8_t kExitPass = 0x10;
inline constexpr uint8_t kExitFail = 0x11;
[[noreturn]] void qemu_exit(uint8_t code);

} // namespace arch
