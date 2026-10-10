// kmain.cc - F4-35: the DR403 kernel's main. One early console (found through /chosen, like
// F4-31), then the whole machine is described by the devicetree and handled by the driver model.
#include "dm.h"

namespace {
uint64_t g_early_uart = 0;
void early_putc(char c)
{
    while ((k::rd32(g_early_uart + 0x18) & (1u << 5)) != 0) {
    }
    k::wr32(g_early_uart, static_cast<uint8_t>(c));
}
bool early_console(const fdt::Tree& t)
{
    fdt::Prop p;
    int chosen = t.find_path("/chosen");
    if (chosen == fdt::kNone || !t.prop(chosen, "stdout-path", p)) {
        return false;
    }
    char path[96];
    uint32_t i = 0;
    for (; i + 1 < sizeof path && i < p.len && p.data[i] != '\0' && p.data[i] != ':'; ++i) {
        path[i] = static_cast<char>(p.data[i]);
    }
    path[i] = '\0';
    int n = t.find_path(path);
    uint64_t size = 0;
    if (n == fdt::kNone || !t.compatible(n, "arm,pl011") || !t.mmio(n, 0, g_early_uart, size)) {
        return false;
    }
    k::set_console(early_putc);
    return true;
}
}  // namespace

extern "C" void kmain(const void* dtb, uint64_t entry_el, uint64_t load_addr)
{
    fdt::Tree t;
    if (!t.init(dtb)) {
        k::exit(2);
    }
    if (!early_console(t)) {
        k::exit(4);
    }
    fdt::Prop model;
    t.prop(t.root(), "model", model);
    uint64_t midr = 0;
    asm volatile("mrs %0, midr_el1" : "=r"(midr));
    k::printf("DR403 kernel on \"%s\": entered at EL%lu, loaded at 0x%lx, DTB at %p\n",
              reinterpret_cast<const char*>(model.data), entry_el, load_addr, dtb);
    uint64_t ram = 0;
    for (int n = t.root(); n != fdt::kNone; n = t.next_node(n)) {
        fdt::Prop dt;
        uint64_t a = 0;
        uint64_t s = 0;
        if (t.prop(n, "device_type", dt) && fdt::streq(reinterpret_cast<const char*>(dt.data), "memory") &&
            t.available(n)) {
            for (int i = 0; t.reg(n, i, a, s); ++i) {
                ram += s;
            }
        }
    }
    int cpus = 0;
    int cn = t.find_path("/cpus");
    for (int c = cn == fdt::kNone ? fdt::kNone : t.first_child(cn); c != fdt::kNone; c = t.next_sibling(c)) {
        fdt::Prop dt;
        cpus += t.prop(c, "device_type", dt) && fdt::streq(reinterpret_cast<const char*>(dt.data), "cpu") ? 1 : 0;
    }
    k::printf("memory %lu MiB, CPUs %d, timer %lu Hz, MIDR_EL1 0x%lx\n", ram >> 20, cpus, k::counter_freq(), midr);
    dm::init(dtb);
    dm::probe_all();
    dm::report();
    k::printf("DR403 kernel: done\n");
    dm::power_off(0);
}
