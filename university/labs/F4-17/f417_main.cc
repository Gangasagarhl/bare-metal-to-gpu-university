// f417_main.cc - F4-17 lab kernel: bind the edu4 driver by ID, probe it, and use it.
#include "k4.h"
#include "pci4.h"
#include "edu4.h"
#include <linux/pci_regs.h>

static edu4::Device g_dev;
static bool g_bound;

static void try_bind(const pci4::Function& f, void*)
{
    if (g_bound || !edu4::matches(f)) return;
    const edu4::Err e = edu4::probe(f, g_dev);
    k4::puts("edu4: probe "); k4::hex(f.at.bus, 2); k4::putc(':'); k4::hex(f.at.dev, 2);
    k4::putc('.'); k4::hex(f.at.fn, 1); k4::puts(": "); k4::line(edu4::to_string(e));
    g_bound = e == edu4::Err::Ok;
}

extern "C" void kmain(uint32_t, uint32_t)
{
    k4::console_init();
    k4::line("F4-17 driver run");
    pci4::enumerate(try_bind, nullptr);
    if (!g_bound) {
        k4::line("edu4: not bound");
        k4::exit_qemu(0x11);
    }
    int bad = 0;
    uint32_t expect = 1;                                 // n! in 32-bit arithmetic
    for (uint32_t n = 0; n <= 14; ++n) {
        if (n > 0) expect *= n;
#ifdef F417_REMOVE_AT
        if (n == F417_REMOVE_AT) {   // fault injection: the device stops answering (decoding off)
            const uint16_t cmd = pci4::read16(g_dev.at, PCI_COMMAND);
            pci4::write32(g_dev.at, PCI_COMMAND, cmd & ~PCI_COMMAND_MEMORY);
            k4::line("fault injection: memory decoding switched off (the device is \"gone\")");
        }
#endif
        uint32_t r = 0;
        const edu4::Err e = edu4::compute(g_dev, n, r);
        k4::puts("compute("); k4::dec(n); k4::puts(") = ");
        if (e != edu4::Err::Ok) { k4::line(edu4::to_string(e)); ++bad; break; }
        k4::dec(r);
        k4::puts(r == expect ? "  (= n! modulo 2^32)" : "  DIFFERS from n! modulo 2^32");
        k4::putc('\n');
        bad += r != expect;
    }
    edu4::dump(g_dev);
    k4::puts("result: "); k4::dec(static_cast<uint64_t>(bad)); k4::line(" problem(s)");
    k4::exit_qemu(bad ? 0x11 : 0x10);
}
