// sbi.h - F4-28: calls from S-mode into the M-mode firmware through the Supervisor Binary
// Interface. Extension and function IDs after the RISC-V SBI specification (title only,
// pending verification); the lab run probes each extension and prints what OpenSBI answers.
#pragma once
#include <cstdint>

namespace sbi {

struct Ret {
    int64_t error;   // 0 = success; negative = SBI error code
    int64_t value;
};

inline constexpr uint64_t kBase = 0x10;
inline constexpr uint64_t kTime = 0x54494D45;    // "TIME"
inline constexpr uint64_t kIpi = 0x735049;       // "sPI"
inline constexpr uint64_t kRfence = 0x52464E43;  // "RFNC"
inline constexpr uint64_t kHsm = 0x48534D;       // "HSM"
inline constexpr uint64_t kSrst = 0x53525354;    // "SRST"
inline constexpr uint64_t kDbcn = 0x4442434E;    // "DBCN" (debug console)
inline constexpr uint64_t kLegacyPutchar = 0x01;

// ecall: extension ID in a7, function ID in a6, arguments in a0..a2; error in a0, value in a1.
inline Ret call(uint64_t ext, uint64_t fid, uint64_t a0 = 0, uint64_t a1 = 0, uint64_t a2 = 0)
{
    register uint64_t r0 asm("a0") = a0;
    register uint64_t r1 asm("a1") = a1;
    register uint64_t r2 asm("a2") = a2;
    register uint64_t r6 asm("a6") = fid;
    register uint64_t r7 asm("a7") = ext;
    asm volatile("ecall" : "+r"(r0), "+r"(r1) : "r"(r2), "r"(r6), "r"(r7) : "memory");
    return Ret{static_cast<int64_t>(r0), static_cast<int64_t>(r1)};
}

inline bool probe(uint64_t ext)                  // base function 3: probe_extension
{
    return call(kBase, 3, ext).value != 0;
}
inline int64_t spec_version() { return call(kBase, 0).value; }
inline int64_t impl_id() { return call(kBase, 1).value; }
inline int64_t impl_version() { return call(kBase, 2).value; }
inline Ret set_timer(uint64_t when) { return call(kTime, 0, when); }
inline Ret send_ipi(uint64_t hart_mask, uint64_t hart_mask_base) { return call(kIpi, 0, hart_mask, hart_mask_base); }
inline Ret hart_start(uint64_t hart, uint64_t start, uint64_t opaque) { return call(kHsm, 0, hart, start, opaque); }
inline Ret hart_status(uint64_t hart) { return call(kHsm, 2, hart); }
[[noreturn]] void shutdown();                    // SRST: type 0 = shutdown, reason 0 = none

} // namespace sbi
