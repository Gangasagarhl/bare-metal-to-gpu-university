// f416_main.cc - F4-16 lab kernel: a read-only register dump of the device's first 256 bytes
// of BAR0, then the "working driver" runs its self-test, then the same dump again. QEMU traces
// every access at the hardware boundary (run.sh: -trace memory_region_ops_*).
#include "k4.h"
#include "pci4.h"
#include <linux/pci_regs.h>

int vendor_driver_selftest();

static pci4::Addr g_at;
static bool g_found;
static void match(const pci4::Function& f, void*)
{
    if (f.vendor == 0x1234 && f.device == 0x11E8 && !g_found) { g_at = f.at; g_found = true; }
}

static void dump(const char* tag, uint32_t base, uint32_t out[64])
{
    k4::puts(tag); k4::putc('\n');
    for (uint32_t i = 0; i < 64; ++i) {
        out[i] = k4::mmio_read32(base + 4 * i);
        if (i % 8 == 0) { k4::puts("  "); k4::hex(4 * i, 2); k4::puts(":"); }
        k4::putc(' '); k4::hex(out[i], 8);
        if (i % 8 == 7) k4::putc('\n');
    }
}

extern "C" void kmain(uint32_t, uint32_t)
{
    k4::console_init();
    k4::line("F4-16 observation run");
    pci4::enumerate(match, nullptr);
    if (!g_found) k4::panic("no 1234:11e8 device");
    const uint16_t cmd = pci4::read16(g_at, PCI_COMMAND);
    pci4::write32(g_at, PCI_COMMAND, cmd | PCI_COMMAND_MEMORY);
    const uint32_t base = pci4::read32(g_at, PCI_BASE_ADDRESS_0) & 0xFFFFFFF0u;
    static uint32_t before[64], after[64];
    k4::line("== marker: read-only dump 1 ==");
    dump("dump before (offset: values, 32-bit reads)", base, before);
    k4::line("== marker: driver self-test ==");
    const int rc = vendor_driver_selftest();
    k4::line("== marker: read-only dump 2 ==");
    dump("dump after", base, after);
    k4::line("differences between the two dumps:");
    for (int i = 0; i < 64; ++i) {
        if (before[i] == after[i]) continue;
        k4::puts("  offset 0x"); k4::hex(4 * i, 2); k4::puts(": 0x"); k4::hex(before[i], 8);
        k4::puts(" -> 0x"); k4::hex(after[i], 8); k4::putc('\n');
    }
    k4::puts("self-test result "); k4::dec(static_cast<uint64_t>(rc)); k4::putc('\n');
    k4::exit_qemu(rc == 0 ? 0x10 : 0x11);
}
