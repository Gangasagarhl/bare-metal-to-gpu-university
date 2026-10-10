// monitor.cc - F11-05 Listing 1: a tiny secure monitor at EL3. It keeps a secret key in secure
// RAM, offers one service through SMC (a keyed digest of a 64-bit message), and drops to the
// normal world. A teaching model of what Trusted Firmware-A's EL3 runtime and a trusted OS do;
// it is not TF-A and implements none of its interfaces.
#include "sha256.h"
#include "tz.h"

extern "C" uint64_t semihost_call(uint64_t op, uint64_t arg);

namespace {

// The key lives in secure RAM (linker section .secure_bss): the normal world cannot read it.
[[gnu::section(".secure_bss")]] uint8_t g_key[32];

uint64_t mac(uint64_t message)
{
    Sha256 h;
    h.update(g_key, sizeof g_key);
    uint8_t m[8];
    for (int i = 0; i < 8; ++i) { m[i] = static_cast<uint8_t>(message >> (8 * i)); }
    h.update(m, sizeof m);
    uint8_t d[32];
    h.finish(d);
    uint64_t out = 0;
    for (int i = 0; i < 8; ++i) { out = (out << 8) | d[i]; }
    return out;
}

[[noreturn]] void qemu_exit(uint64_t code)
{
    static uint64_t block[2];
    block[0] = 0x20026;           // ADP_Stopped_ApplicationExit (semihosting specification)
    block[1] = code;
    semihost_call(0x18, reinterpret_cast<uint64_t>(block));   // SYS_EXIT
    for (;;) {
    }
}

uint64_t read_el()
{
    uint64_t v;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(v));
    return (v >> 2) & 3;
}

}  // namespace


extern "C" uint64_t el3_main()
{
    tz::print("[EL3 monitor] running at EL");
    tz::putc(static_cast<char>('0' + read_el()));
    tz::print(", secure state; key stored at ");
    tz::hex(reinterpret_cast<uint64_t>(g_key), 8);
    tz::print(" (secure RAM)\n");
    for (int i = 0; i < 32; ++i) {
        g_key[i] = static_cast<uint8_t>(0xA5 ^ (i * 7));   // a fixed demo key: never do this
    }
#ifdef CONTROL_SECURE_EL1
    tz::print("[EL3 monitor] CONTROL RUN: dropping to SECURE EL1 (SCR_EL3.NS = 0) at 0x40200000\n");
#else
    tz::print("[EL3 monitor] dropping to the normal world (SCR_EL3.NS = 1) at 0x40200000\n");
#endif
    return 0x40200000;
}

extern "C" void el3_smc(uint64_t* regs)
{
    uint64_t esr;
    __asm__ volatile("mrs %0, esr_el3" : "=r"(esr));
    const uint64_t ec = esr >> 26;          // exception class; 0x17 = SMC from AArch64
    if (ec != 0x17) {
        tz::print("[EL3 monitor] unexpected exception, ESR_EL3 = ");
        tz::hex(esr, 8);
        tz::print("\n");
        qemu_exit(2);
    }
    switch (regs[0]) {
    case tz::kSmcVersion:
        regs[0] = 1;
        break;
    case tz::kSmcMac:
        tz::print("[EL3 monitor] SMC: keyed digest requested for ");
        tz::hex(regs[1]);
        tz::print("\n");
        regs[0] = mac(regs[1]);
        break;
    case tz::kSmcExit:
        tz::print("[EL3 monitor] SMC: exit requested, code ");
        tz::hex(regs[1], 2);
        tz::print("\n");
        qemu_exit(regs[1]);
#ifdef WITH_PEEK
    case tz::kSmcPeek:                      // a debug helper left in the monitor
        tz::print("[EL3 monitor] SMC: peek at ");
        tz::hex(regs[1], 8);
        tz::print("\n");
#ifdef PEEK_CHECKED
        if (regs[1] < tz::kNormalRam || regs[1] + 8 > tz::kNormalRamEnd) {   // caller's memory only
            regs[0] = tz::kSmcUnknown;
            break;
        }
#endif
        regs[0] = *reinterpret_cast<volatile uint64_t*>(regs[1]);
        break;
#endif
    default:
        regs[0] = tz::kSmcUnknown;
        break;
    }
}

extern "C" void el3_panic()
{
    tz::print("[EL3 monitor] unexpected exception\n");
    qemu_exit(3);
}
