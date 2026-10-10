// f415_main.cc - F4-15 lab kernel: a boot log in which every PCI function is either claimed by
// a driver from the match table or reported as "no driver", with a TSC timestamp per line.
// This log is the "boot trace" of the forensic lab.
#include "k4.h"
#include "pci4.h"
#include "bind4.h"

static uint64_t g_t0;

static void stamp()
{
    k4::puts("[tsc+");
    k4::dec(k4::rdtsc() - g_t0);
    k4::puts("] ");
}

static void bind_one(const pci4::Function& f, void*)
{
    stamp();
    const bind4::Driver* d = bind4::find(f);
    k4::puts(d ? "bind " : "NO DRIVER ");
    k4::hex(f.at.bus, 2); k4::putc(':'); k4::hex(f.at.dev, 2); k4::putc('.'); k4::hex(f.at.fn, 1);
    k4::puts(" "); k4::hex(f.vendor, 4); k4::putc(':'); k4::hex(f.device, 4);
    k4::puts(" class "); k4::hex(f.base_class, 2); k4::putc(' '); k4::hex(f.sub_class, 2);
    k4::putc(' '); k4::hex(f.prog_if, 2);
    if (d) {
        k4::puts(" -> "); k4::puts(d->name); k4::puts(" ["); k4::puts(d->standard); k4::putc(']');
    } else {
        k4::puts(" sub "); k4::hex(f.subvendor, 4); k4::putc(':'); k4::hex(f.subdevice, 4);
        k4::puts(" rev "); k4::hex(f.revision, 2);
    }
    k4::putc('\n');
}

extern "C" void kmain(uint32_t, uint32_t)
{
    g_t0 = k4::rdtsc();
    k4::console_init();
    stamp(); k4::line("F4-15 boot: console up");
    const int n = pci4::enumerate(bind_one, nullptr);
    stamp(); k4::puts("boot: "); k4::dec(static_cast<uint64_t>(n)); k4::line(" PCI functions seen");
    k4::exit_qemu(0x10);
}
