// hello.cc - F4-31: a kernel that knows nothing about the machine except what the devicetree
// tells it. It finds its console, then reports memory, CPUs, the timer and every device node.
#include "fdt.h"
#include "kbase.h"

namespace {
uint64_t g_uart = 0;

// PL011: data register at offset 0x00; flag register at 0x18, bit 5 = transmit FIFO full
// (Arm PrimeCell UART (PL011) Technical Reference Manual, register summary - title only,
// pending verification).
void pl011_putc(char c)
{
    while ((k::rd32(g_uart + 0x18) & (1u << 5)) != 0) {
    }
    k::wr32(g_uart, static_cast<uint8_t>(c));
}

// The console: the node named by /chosen/stdout-path if there is one, else the first PL011.
int find_console(const fdt::Tree& t)
{
    int chosen = t.find_path("/chosen");
    fdt::Prop p;
    if (chosen != fdt::kNone && t.prop(chosen, "stdout-path", p)) {
        char path[96];
        uint32_t i = 0;
        for (; i + 1 < sizeof path && i < p.len && p.data[i] != '\0' && p.data[i] != ':'; ++i) {
            path[i] = static_cast<char>(p.data[i]);
        }
        path[i] = '\0';
        int n = t.find_path(path);
        if (n != fdt::kNone && t.compatible(n, "arm,pl011")) {
            return n;
        }
    }
    for (int n = t.root(); n != fdt::kNone; n = t.next_node(n)) {
        if (t.compatible(n, "arm,pl011")) {
            return n;
        }
    }
    return fdt::kNone;
}
}  // namespace

extern "C" void kmain(const void* dtb, uint64_t entry_el, uint64_t load_addr)
{
    fdt::Tree t;
    if (!t.init(dtb)) {
        k::exit(2);                       // no devicetree: nowhere to print, so only an exit code
    }
    int con = find_console(t);
    uint64_t size = 0;
    if (con == fdt::kNone || !t.mmio(con, 0, g_uart, size)) {
        k::exit(4);
    }
    k::set_console(pl011_putc);
    char path[96];
    t.path(con, path, sizeof path);
    k::printf("F4-31: console is %s (arm,pl011) at 0x%lx, found in the devicetree\n", path, g_uart);
    k::printf("entered at EL%u, image loaded at 0x%lx, DTB at %p (%u bytes)\n",
              static_cast<unsigned>(entry_el), load_addr, dtb, t.total_size());
    fdt::Prop model;
    if (t.prop(t.root(), "model", model)) {
        k::printf("model: %s\n", reinterpret_cast<const char*>(model.data));
    }
    uint64_t ram = 0;
    for (int n = t.root(); n != fdt::kNone; n = t.next_node(n)) {
        fdt::Prop dt;
        if (t.prop(n, "device_type", dt) && fdt::streq(reinterpret_cast<const char*>(dt.data), "memory")) {
            uint64_t a = 0;
            uint64_t s = 0;
            for (int i = 0; t.reg(n, i, a, s); ++i) {
                k::printf("memory: 0x%lx bytes at 0x%lx\n", s, a);
                ram += s;
            }
        }
    }
    int cpus = 0;
    int cpus_node = t.find_path("/cpus");
    for (int c = t.first_child(cpus_node); cpus_node != fdt::kNone && c != fdt::kNone; c = t.next_sibling(c)) {
        fdt::Prop dt;
        if (t.prop(c, "device_type", dt) && fdt::streq(reinterpret_cast<const char*>(dt.data), "cpu")) {
            ++cpus;
        }
    }
    k::printf("total memory %lu MiB, CPUs %d, counter frequency %lu Hz (CNTFRQ_EL0)\n",
              ram >> 20, cpus, k::counter_freq());
    int devices = 0;
    for (int n = t.root(); n != fdt::kNone; n = t.next_node(n)) {
        fdt::Prop c;
        uint64_t a = 0;
        uint64_t s = 0;
        if (t.prop(n, "compatible", c) && t.mmio(n, 0, a, s)) {
            ++devices;
            if (!t.compatible(n, "virtio,mmio")) {   // 32 identical transports: shown as a count
                t.path(n, path, sizeof path);
                k::printf("  device %-28s %-26s at 0x%lx\n", path, reinterpret_cast<const char*>(c.data), a);
            }
        }
    }
    k::printf("%d device nodes with an address (virtio,mmio transports not listed one by one)\n", devices);
    k::printf("F4-31 ok\n");
    k::exit(0);
}
