// f444_main.cc - DR405 F4-44: inspect every display-class PCI function, read-only.
#include "../F4-01/kbase.h"
#include "gpuprobe.h"

namespace {
int g_found = 0;
void visit(PciAddr a)
{
    if ((pci::read32(a, pcireg::CLASS_REVISION) >> 24) != 0x03) return;   // base class 3: display
    ++g_found;
    gpuprobe::inspect(a);
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t)
{
    serial_init();
    kprintf("F4-44 kernel: magic=0x%x; read-only display probe\n", magic);
    pci::enumerate(visit);
    gpuprobe::legacy_shadow();
    kprintf("display functions: %d; configuration writes: %d (ROM BAR enable + restore); "
            "writes to device registers: 0\n", g_found, gpuprobe::config_writes());
    qemu_exit(g_found ? 0x10 : 0x01);
}
