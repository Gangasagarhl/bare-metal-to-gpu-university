// blindprobe.cc - F4-31 forensic evidence: a "driver" ported from a PC habit. Instead of reading
// the devicetree it scans the address space for anything that answers like an Arm PrimeCell
// (identification registers at offsets 0xFF0-0xFFC), the way PCI enumeration scans for vendor
// IDs. The scan step and range were chosen by the person who wrote it; see the forensic lab.
#include "kbase.h"

namespace {
uint64_t g_uart = 0x09000000;   // hard-coded: only right for one machine
void putc_fixed(char c)
{
    while ((k::rd32(g_uart + 0x18) & (1u << 5)) != 0) {
    }
    k::wr32(g_uart, static_cast<uint8_t>(c));
}
}  // namespace

extern "C" void kmain(const void*, uint64_t, uint64_t)
{
    k::set_console(putc_fixed);
    k::printf("blindprobe: scanning 0x08000000-0x0a000000 in 64 KiB steps for PrimeCell IDs\n");
    for (uint64_t a = 0x08000000; a < 0x0a000000; a += 0x10000) {
        uint32_t id = 0;
        for (int i = 0; i < 4; ++i) {
            id |= (k::rd32(a + 0xff0 + 4 * i) & 0xff) << (8 * i);
        }
        k::printf("  0x%08lx: cell id 0x%08x%s\n", a, id, id == 0xb105f00d ? "  <- PrimeCell" : "");
    }
    k::printf("blindprobe: scan finished\n");
    k::exit(0);
}
