// kmain.cpp: privileged instructions through small wrappers, in ring 0 under QEMU.
#include "console.h"
#include "cpu32.h"

extern "C" void kmain()
{
    print("F2-49 kernel: privileged wrappers in ring 0\n");
    const uint32_t cr0 = cpu::readCr0();
    print("CR0    = ");
    printHex(cr0);
    print("  (bit 0 = ");
    printDec(cr0 & 1u);
    print(", bit 31 = ");
    printDec(cr0 >> 31);
    print(")\n");

    cpu::cli();
    const uint32_t flags = cpu::readEflags();
    print("EFLAGS = ");
    printHex(flags);
    print("  (bit 9 after cli = ");
    printDec((flags >> 9) & 1u);
    print(")\n");

    // isa-debug-exit is write-only; reading an unused port is harmless in QEMU.
    print("inb(0xf4) = ");
    printHex(cpu::inb(0xf4));
    print("\n");
    qemuExit(0x10);
}
