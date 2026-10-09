// kmain.cpp: find QEMU's edu device on PCI bus 0, then drive its registers through the
// typed volatile wrappers of mmio.h.
#include "console.h"
#include "edu.h"
#include "pci.h"

namespace {

void line(const char* label, uint32_t value)
{
    print(label);
    printHex(value);
    print("\n");
}

}  // namespace

extern "C" void kmain()
{
    print("F2-50 kernel: volatile and memory-mapped registers\n");
    uintptr_t base = 0;
    for (uint32_t dev = 0; dev < 32 && base == 0; ++dev) {
        const uint32_t id = pci::read32(0, dev, 0, 0x00);   // device id << 16 | vendor id
        if (id == 0x11e81234u) {
            print("edu found at bus 0, device ");
            printDec(dev);
            print("\n");
            const uint32_t bar0 = pci::read32(0, dev, 0, 0x10);
            line("BAR0 register      = ", bar0);
            base = bar0 & ~0xfu;   // the low 4 bits describe the BAR, not the address
        }
    }
    if (base == 0) {
        print("edu device not found\n");
        qemuExit(0x11);
    }

    line("identification     = ", edu::Identification::read(base));
    edu::Liveness::write(base, 0x12345678u);
    line("liveness(0x12345678) -> ", edu::Liveness::read(base));

    edu::Factorial::write(base, 10);
    uint32_t polls = 0;
    while (edu::Status::read(base) & edu::kStatusComputing) {   // volatile read every time
        ++polls;
    }
    print("status polls while computing: ");
    print(polls > 0 ? "at least one\n" : "none (already done)\n");
    print("factorial(10)      = ");
    printDec(edu::Factorial::read(base));
    print("\n");
    qemuExit(0x10);
}
