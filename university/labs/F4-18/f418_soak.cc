// f418_soak.cc - F4-18 soak test in QEMU: 20,000 computations through the edu4 driver,
// every result checked, counters printed at the end.
#include "k4.h"
#include "pci4.h"
#include "edu4.h"

static edu4::Device g_dev;
static bool g_bound;
static void try_bind(const pci4::Function& f, void*)
{
    if (!g_bound && edu4::matches(f)) g_bound = edu4::probe(f, g_dev) == edu4::Err::Ok;
}

extern "C" void kmain(uint32_t, uint32_t)
{
    k4::console_init();
    k4::line("F4-18 soak: 20000 computations");
    pci4::enumerate(try_bind, nullptr);
    if (!g_bound) { k4::line("soak: driver not bound"); k4::exit_qemu(0x11); }
    uint32_t table[15];
    table[0] = 1;
    for (uint32_t n = 1; n < 15; ++n) table[n] = table[n - 1] * n;
    uint32_t wrong = 0, errors = 0;
    const uint64_t t0 = k4::rdtsc();
    for (uint32_t i = 0; i < 20000; ++i) {
        const uint32_t n = i % 15;
        uint32_t r = 0;
        if (edu4::compute(g_dev, n, r) != edu4::Err::Ok) ++errors;
        else if (r != table[n]) ++wrong;
    }
    const uint64_t ticks = k4::rdtsc() - t0;
    edu4::dump(g_dev);
    k4::puts("soak: errors "); k4::dec(errors); k4::puts(", wrong results "); k4::dec(wrong);
    k4::puts(", time-stamp counter ticks "); k4::dec(ticks); k4::putc('\n');
    k4::exit_qemu(errors || wrong ? 0x11 : 0x10);
}
