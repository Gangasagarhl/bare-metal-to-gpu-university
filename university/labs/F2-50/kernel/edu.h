// edu.h: register map of QEMU's "edu" teaching device, as used in this lab.
// The offsets and meanings are written from memory of QEMU's edu documentation and are NOT
// verified against it in this build (chapter F2-50, unverified box); the lab run shows the
// behaviour QEMU 8.2.2 actually had.
#pragma once
#include "mmio.h"

namespace edu {

using Identification = Reg<uint32_t, 0x00, Access::ReadOnly>;   // reads 0xRRrr00ed
using Liveness       = Reg<uint32_t, 0x04>;                     // reads back the inverse
using Factorial      = Reg<uint32_t, 0x08>;                     // write n, later read n!
using Status         = Reg<uint32_t, 0x20>;                     // bit 0: computing

inline constexpr uint32_t kStatusComputing = 0x1;

}  // namespace edu
