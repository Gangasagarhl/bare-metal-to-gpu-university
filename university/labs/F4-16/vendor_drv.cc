// vendor_drv.cc - the "working driver" whose behaviour F4-16 observes. In the lab story this is
// a closed binary from a vendor; the observer is not allowed to read this file and learns only
// what the QEMU trace shows at the hardware boundary. (It is shipped as source here so that the
// course can be rebuilt; reading it before finishing the lab spoils the exercise.)
// Built with -DF416_FORGET_ACK it becomes the learner's first driver of the forensic lab.
#include "k4.h"
#include "pci4.h"
#include <linux/pci_regs.h>

static uint32_t g_base;
static pci4::Addr g_at;
static bool g_found;

static void match(const pci4::Function& f, void*)
{
    if (f.vendor == 0x1234 && f.device == 0x11E8 && !g_found) {
        g_at = f.at;
        g_found = true;
    }
}

static uint32_t rd(uint32_t off) { return k4::mmio_read32(g_base + off); }
static void wr(uint32_t off, uint32_t v) { k4::mmio_write32(g_base + off, v); }

static bool compute(uint32_t n, uint32_t& result)
{
    wr(0x08, n);
    for (int i = 0; i < 100000; ++i) {
        if ((rd(0x20) & 1) == 0) {
            result = rd(0x08);
            return true;
        }
    }
    return false;
}

// Returns 0 on success; prints one line per step.
int vendor_driver_selftest()
{
    pci4::enumerate(match, nullptr);
    if (!g_found) { k4::line("drv: device not found"); return 1; }
    const uint16_t cmd = pci4::read16(g_at, PCI_COMMAND);
    pci4::write32(g_at, PCI_COMMAND, cmd | PCI_COMMAND_MEMORY);
    g_base = pci4::read32(g_at, PCI_BASE_ADDRESS_0) & 0xFFFFFFF0u;
    k4::puts("drv: id register 0x"); k4::hex(rd(0x00), 8); k4::putc('\n');
    wr(0x04, 0x13579BDF);
    k4::puts("drv: liveness 0x13579bdf -> 0x"); k4::hex(rd(0x04), 8); k4::putc('\n');
    static const uint32_t kInputs[] = {5, 10};
    for (uint32_t n : kInputs) {
        uint32_t r = 0;
        if (!compute(n, r)) { k4::line("drv: timeout"); return 2; }
        k4::puts("drv: compute("); k4::dec(n); k4::puts(") = "); k4::dec(r); k4::putc('\n');
    }
    wr(0x60, 0x42);
    k4::puts("drv: after raise, 0x24 reads 0x"); k4::hex(rd(0x24), 8); k4::putc('\n');
#ifndef F416_FORGET_ACK
    wr(0x64, 0x42);
#endif
    const uint32_t left = rd(0x24);
    k4::puts("drv: at the end, 0x24 reads 0x"); k4::hex(left, 8); k4::putc('\n');
    return left == 0 ? 0 : 3;
}
