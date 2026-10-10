// psci.h - F4-24: calls into firmware through the Arm Power State Coordination Interface.
// Function IDs after the PSCI specification and the SMC Calling Convention (Arm; title only,
// pending verification). QEMU virt implements PSCI itself; the devicetree's /psci node says
// whether the call instruction is HVC or SMC (F4-23 checklist row 2).
#pragma once
#include <cstdint>
#include "fdt.h"

namespace psci {

inline constexpr uint32_t kVersion = 0x84000000;
inline constexpr uint32_t kCpuOn64 = 0xc4000003;
inline constexpr uint32_t kAffinityInfo64 = 0xc4000004;
inline constexpr uint32_t kSystemOff = 0x84000008;
inline constexpr uint32_t kSystemReset = 0x84000009;

bool init(const fdt::Blob& dt);           // reads /psci "method"; false if absent
const char* method();                     // "hvc", "smc" or "none"
int64_t call(uint64_t fn, uint64_t a1 = 0, uint64_t a2 = 0, uint64_t a3 = 0);
[[noreturn]] void system_off();

} // namespace psci
