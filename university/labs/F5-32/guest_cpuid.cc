// guest_cpuid.cc - the same question as hvdetect.cpp, asked by a UEFI program inside a QEMU
// virtual machine, where the answer depends on how QEMU was started (run.sh: TCG emulation,
// TCG with the hypervisor bit removed, and a KVM attempt). Then it runs a fixed amount of
// arithmetic (an integer loop, then a floating-point loop) between printed lines, so that the
// host's time stamps on those lines show how fast this virtual CPU computes each kind of work
// (compare with work_host.cc). A UEFI application (route of F3-10).
#include "console.hpp"
#include "efi.hpp"

namespace {

void cpuid(uint32_t leaf, uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d)
{
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(leaf), "c"(0));
}

uint64_t bits(double d)
{
    uint64_t b = 0;
    __builtin_memcpy(&b, &d, sizeof b);
    return b;
}

}  // namespace

// The Microsoft-style target expects this symbol in any program that uses floating point.
extern "C" int _fltused = 0;

extern "C" efi::Status efi_main(efi::Handle /*image*/, efi::SystemTable* st)
{
    Console con(st->con_out);
    uint32_t a = 0, b = 0, c = 0, d = 0;
    cpuid(1, a, b, c, d);
    const bool hv = ((c >> 31) & 1u) != 0;
    con.print("guest: CPUID leaf 1 ECX bit 31 (hypervisor present) = ");
    con.dec(hv ? 1 : 0);
    con.print("\n");
    cpuid(0x40000000u, a, b, c, d);
    char sig[13] = {};
    const uint32_t regs[3] = {b, c, d};
    for (int r = 0; r < 3; ++r) {
        for (int i = 0; i < 4; ++i) {
            const char ch = static_cast<char>((regs[r] >> (8 * i)) & 0xff);
            sig[r * 4 + i] = (ch >= 32 && ch < 127) ? ch : '.';
        }
    }
    con.print("guest: CPUID leaf 0x40000000 signature \"");
    con.print(sig);
    con.print("\"");
    con.print(hv ? "\n" : " (not meaningful: the hypervisor bit is 0)\n");
    con.print("guest: integer work start\n");
    uint64_t x = 1;
    for (uint64_t i = 0; i < 200000000; ++i) {
        x = x * 6364136223846793005ull + 1442695040888963407ull;   // a 64-bit LCG step
        __asm__ volatile("" : "+r"(x));                                // keep every step
    }
    con.print("guest: integer work end, result ");
    con.hex(x);
    con.print("\nguest: floating-point work start\n");
    double y = 1.0;
    for (int i = 0; i < 50000000; ++i) {
        y = y * 0.999999 + 0.5;                                        // double arithmetic
        __asm__ volatile("" : "+x"(y));
    }
    con.print("guest: floating-point work end, result bits ");
    con.hex(bits(y));
    con.print("\n");
    qemu_exit(0x10);
    return efi::kSuccess;
}
