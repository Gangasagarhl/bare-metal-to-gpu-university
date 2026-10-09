// kmain_fast.cpp: a "faster" version a teammate wrote: plain pointers instead of the
// wrappers, because "volatile makes the code slow".
#include "console.h"
#include "pci.h"

extern "C" void kmain()
{
    print("F2-50 kernel (fast version)\n");
    uintptr_t base = 0;
    for (uint32_t dev = 0; dev < 32 && base == 0; ++dev) {
        if (pci::read32(0, dev, 0, 0x00) == 0x11e81234u) {
            base = pci::read32(0, dev, 0, 0x10) & ~0xfu;
        }
    }
    auto* regs = reinterpret_cast<uint32_t*>(base);   // BUG? no volatile
    regs[0x08 / 4] = 10;                               // start factorial(10)
    while (regs[0x20 / 4] & 0x1) {                     // wait while status says "computing"
    }
    const uint32_t result = regs[0x08 / 4];            // read the result
    print("factorial(10) = ");
    printDec(result);
    print("\n");
    qemuExit(0x10);
}
