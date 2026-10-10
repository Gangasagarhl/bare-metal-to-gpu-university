// tz.h - F11-05: what the secure monitor (EL3) and the normal-world program (non-secure EL1)
// agree on: the SMC function identifiers of our own tiny "secure service", and a PL011 UART
// printer. Addresses are those of QEMU's "virt" board as QEMU 8.2 builds it with secure=on
// (UART0 at 0x09000000, secure RAM at 0x0e000000, normal RAM at 0x40000000), taken from
// QEMU's virt board documentation and source (pending verification); the run shows them working.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace tz {

// SMC function IDs. Bit 31 = fast call, bit 30 = SMC64, bits 29:24 = owning entity
// (3 = "OEM service" range in the SMC Calling Convention; our choice, pending verification).
constexpr uint64_t kSmcVersion = 0xC3000000;   // returns our service's version
constexpr uint64_t kSmcMac = 0xC3000001;       // x1 = message: returns a keyed digest (8 bytes)
constexpr uint64_t kSmcExit = 0xC3000002;      // x1 = exit code: ends the QEMU run
constexpr uint64_t kSmcPeek = 0xC3000003;      // x1 = address: a leftover "debug" service (forensic lab)
constexpr uint64_t kSmcUnknown = 0xFFFFFFFF;   // SMCCC "unknown function" return value (-1)

constexpr uintptr_t kUart = 0x09000000;        // PL011 data register at offset 0
constexpr uintptr_t kUartFlags = kUart + 0x18; // flag register; bit 5 = transmit FIFO full
constexpr uintptr_t kSecureRam = 0x0e000000;   // only reachable from the secure world
constexpr uintptr_t kNormalRam = 0x40000000;   // normal RAM: reachable from both worlds
constexpr uintptr_t kNormalRamEnd = 0x48000000; // end of the 128 MiB QEMU gives by default

inline void putc(char c)
{
    while ((*reinterpret_cast<volatile uint32_t*>(kUartFlags) & (1u << 5)) != 0) {
    }
    *reinterpret_cast<volatile uint32_t*>(kUart) = static_cast<uint8_t>(c);
}

inline void print(const char* s)
{
    for (; *s != '\0'; ++s) {
        putc(*s);
    }
}

inline void hex(uint64_t v, int digits = 16)
{
    print("0x");
    for (int i = digits - 1; i >= 0; --i) {
        putc("0123456789abcdef"[(v >> (4 * i)) & 0xf]);
    }
}

}  // namespace tz
