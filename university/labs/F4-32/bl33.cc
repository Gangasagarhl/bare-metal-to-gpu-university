// bl33.cc - F4-32 stage 3 ("BL33", here directly the kernel). The same position-independent
// entry code as the F4-31 kernel (../F4-31/start.S). It finds its console in the devicetree,
// calls the resident monitor twice through SMC and never returns.
#include "../F4-31/fdt.h"
#include "../F4-31/kbase.h"

namespace {
uint64_t g_uart = 0;
void pl011_putc(char c)
{
    while ((k::rd32(g_uart + 0x18) & (1u << 5)) != 0) {
    }
    k::wr32(g_uart, static_cast<uint8_t>(c));
}
uint64_t smc(uint64_t fid)
{
    register uint64_t x0 asm("x0") = fid;
    asm volatile("smc #0" : "+r"(x0) : : "x1", "x2", "x3", "memory");
    return x0;
}
}  // namespace

extern "C" void kmain(const void* dtb, uint64_t entry_el, uint64_t load_addr)
{
    fdt::Tree t;
    if (!t.init(dtb)) {
        k::exit(2);
    }
    for (int n = t.root(); n != fdt::kNone; n = t.next_node(n)) {
        uint64_t size = 0;
        if (t.compatible(n, "arm,pl011") && t.available(n) && t.mmio(n, 0, g_uart, size)) {
            break;                              // first PL011 whose status allows its use
        }
    }
    if (g_uart == 0) {
        k::exit(4);
    }
    k::set_console(pl011_putc);
    k::printf("BL33: kernel entered at EL%lu, loaded at 0x%lx, console 0x%lx from the DTB\n", entry_el,
              load_addr, g_uart);
    k::printf("BL33: devicetree has a /psci node: %s\n", t.find_path("/psci") != fdt::kNone ? "yes" : "no");
    uint64_t v = smc(0x84000000);
    k::printf("BL33: PSCI_VERSION returned 0x%lx (major %lu, minor %lu)\n", v, v >> 16, v & 0xffff);
    k::printf("BL33: asking the firmware to power off\n");
    smc(0x84000008);
    k::printf("BL33: still running after SYSTEM_OFF - firmware did not power off\n");
    k::exit(8);
}
