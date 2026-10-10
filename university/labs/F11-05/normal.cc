// normal.cc - F11-05 Listing 2: the normal-world program at non-secure EL1. It uses the secure
// service through SMC, then tries to read the secret key directly from secure RAM.
#include "tz.h"

extern "C" uint64_t smc_call(uint64_t function_id, uint64_t argument);

namespace {

uint64_t read_el()
{
    uint64_t v;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(v));
    return (v >> 2) & 3;
}

}  // namespace

#ifdef ATTACK
// forensic build: the program a curious user ran, asking the monitor's peek service for the key
extern "C" void ns_main()
{
    tz::print("[normal world] running at EL");
    tz::putc(static_cast<char>('0' + read_el()));
    tz::print("\n");
    uint64_t words[4];
    for (int i = 0; i < 4; ++i) {
        words[i] = smc_call(tz::kSmcPeek, tz::kSecureRam + 8 * i);
    }
    tz::print("[normal world] key bytes via SMC peek:");
    for (const uint64_t w : words) {
        tz::print(" ");
        tz::hex(w);
    }
    tz::print("\n");
    smc_call(tz::kSmcExit, 0);
}
#else
extern "C" void ns_main()
{
    tz::print("[normal world] running at EL");
    tz::putc(static_cast<char>('0' + read_el()));
    tz::print("\n");
    tz::print("[normal world] service version: ");
    tz::hex(smc_call(tz::kSmcVersion, 0), 2);
    tz::print("\n");
    const uint64_t m1 = smc_call(tz::kSmcMac, 0x1234);
    const uint64_t m2 = smc_call(tz::kSmcMac, 0x1234);
    const uint64_t m3 = smc_call(tz::kSmcMac, 0x1235);
    tz::print("[normal world] digest(0x1234) = ");
    tz::hex(m1);
    tz::print(", again = ");
    tz::hex(m2);
    tz::print("\n[normal world] digest(0x1235) = ");
    tz::hex(m3);
    tz::print("\n[normal world] unknown function returns ");
    tz::hex(smc_call(0xC30000FF, 0));
    tz::print("\n[normal world] now reading the key directly at ");
    tz::hex(tz::kSecureRam, 8);
    tz::print(" ...\n");
    const uint64_t stolen = *reinterpret_cast<volatile uint64_t*>(tz::kSecureRam);
    tz::print("[normal world] read succeeded: ");   // reached only if the read was allowed
    tz::hex(stolen);
    tz::print("\n");
    smc_call(tz::kSmcExit, 1);
}
#endif

extern "C" [[noreturn]] void ns_fault(uint64_t esr, uint64_t far, uint64_t elr)
{
    tz::print("[normal world] synchronous exception: ESR_EL1 = ");
    tz::hex(esr, 8);
    tz::print(" (EC ");
    tz::hex(esr >> 26, 2);
    tz::print(", DFSC ");
    tz::hex(esr & 0x3f, 2);
    tz::print("), FAR_EL1 = ");
    tz::hex(far, 8);
    tz::print(", ELR_EL1 = ");
    tz::hex(elr, 8);
    tz::print("\n");
    smc_call(tz::kSmcExit, 0);
    for (;;) {
    }
}
