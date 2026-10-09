// gdt.cc - F3-19: build and load the GDT and the TSS with three IST stacks.
#include "gdt.h"
#include "kprint.h"

namespace gdt {
namespace {

alignas(16) uint64_t g_gdt[7];
Tss g_tss;
alignas(16) uint8_t g_ist_stacks[3][16384];   // double fault, NMI, machine check

struct [[gnu::packed]] Pointer {
    uint16_t limit;
    uint64_t base;
};

} // namespace

Tss& tss()
{
    return g_tss;
}

void init(bool use_ist)
{
    g_gdt[0] = 0;
    g_gdt[1] = segment(kAccKernelCode, kFlagsCode64);
    g_gdt[2] = segment(kAccKernelData, kFlagsData);
    g_gdt[3] = segment(kAccUserData, kFlagsData);
    g_gdt[4] = segment(kAccUserCode, kFlagsCode64);
    // The TSS descriptor is 16 bytes in 64-bit mode: base split over both halves, type 0x9
    // (available 64-bit TSS), present.
    uint64_t base = reinterpret_cast<uint64_t>(&g_tss);
    uint64_t limit = sizeof(Tss) - 1;
    g_gdt[5] = (limit & 0xFFFF) | ((base & 0xFFFFFF) << 16) | (0x89ull << 40) |
               (((limit >> 16) & 0xF) << 48) | (((base >> 24) & 0xFF) << 56);
    g_gdt[6] = base >> 32;

    g_tss.iomap_base = sizeof(Tss);   // no I/O permission bitmap
    if (use_ist) {
        for (int i = 0; i < 3; ++i) {
            g_tss.ist[i] = reinterpret_cast<uint64_t>(&g_ist_stacks[i][sizeof g_ist_stacks[i]]);
        }
    }

    Pointer p{sizeof(g_gdt) - 1, reinterpret_cast<uint64_t>(g_gdt)};
    asm volatile(
        "lgdt %0\n\t"
        "pushq %1\n\t"                  // reload CS with a far return: push selector, target
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n"
        "1:\n\t"
        "movw %2, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%ss\n\t"
        "ltr %w3\n\t"
        :
        : "m"(p), "i"(uint64_t{kKernelCode}), "i"(kKernelData), "r"(kTss)
        : "rax", "memory");
    kprintf("gdt: loaded %d descriptors at %p, TSS at %p, IST1 top %p\n",
            static_cast<int>(sizeof(g_gdt) / 8), static_cast<void*>(g_gdt),
            static_cast<void*>(&g_tss), reinterpret_cast<void*>(g_tss.ist[0]));
}

} // namespace gdt
