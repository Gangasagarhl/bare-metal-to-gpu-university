// desc_host.cpp - F3-19: host unit test of descriptor encodings and error-code decoding.
// The kernel's encoder (gdt.h) must reproduce the two descriptors written by hand in
// F3-18's boot.S, and a decoder must read back every field of a 64-bit interrupt gate.
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include "gdt.h"

int g_fail = 0;

void expect(const char* what, uint64_t got, uint64_t want)
{
    bool ok = got == want;
    std::printf("%-4s %-34s got 0x%016llx want 0x%016llx\n", ok ? "ok" : "FAIL", what,
                static_cast<unsigned long long>(got), static_cast<unsigned long long>(want));
    g_fail += ok ? 0 : 1;
}

int main()
{
    // The values boot.S uses for its temporary GDT (F3-18, Listing 1).
    expect("kernel code (boot.S 0x08)", gdt::segment(gdt::kAccKernelCode, gdt::kFlagsCode64),
           0x00AF9A000000FFFFull);
    expect("kernel data (boot.S 0x10)", gdt::segment(gdt::kAccKernelData, gdt::kFlagsData),
           0x00CF92000000FFFFull);
    // Field extraction from a user code descriptor: DPL lives in access-byte bits 5-6.
    uint64_t uc = gdt::segment(gdt::kAccUserCode, gdt::kFlagsCode64);
    expect("user code DPL", (uc >> 45) & 3, 3);
    expect("user code L bit (bit 53)", (uc >> 53) & 1, 1);
    // A 64-bit interrupt gate for handler 0xffffffff8010f000, selector 0x08, IST 1:
    uint64_t handler = 0xffffffff8010f000ull;
    uint64_t low = (handler & 0xFFFF) | (uint64_t{gdt::kKernelCode} << 16) | (uint64_t{1} << 32) |
                   (uint64_t{0x8E} << 40) | (((handler >> 16) & 0xFFFF) << 48);
    uint64_t high = handler >> 32;
    uint64_t back = (low & 0xFFFF) | (((low >> 48) & 0xFFFF) << 16) | (high << 32);
    expect("gate: handler address round trip", back, handler);
    expect("gate: IST field (bits 32-34)", (low >> 32) & 7, 1);
    expect("gate: type (bits 40-43)", (low >> 40) & 0xF, 0xE);
    expect("gate: present (bit 47)", (low >> 47) & 1, 1);
    // Page-fault error codes seen in this chapter's run: 0x0, 0x2 and the forensic 0x2.
    for (uint64_t e : {0x0ull, 0x2ull, 0x3ull, 0x11ull}) {
        std::printf("     error code 0x%02llx: %s, %s, %s%s\n", static_cast<unsigned long long>(e),
                    (e & 1) ? "protection violation" : "page not present", (e & 2) ? "write" : "read",
                    (e & 4) ? "user" : "supervisor", (e & 16) ? ", instruction fetch" : "");
    }
    std::printf("%s: %d failure(s)\n", g_fail == 0 ? "PASS" : "FAIL", g_fail);
    return g_fail == 0 ? 0 : 1;
}
