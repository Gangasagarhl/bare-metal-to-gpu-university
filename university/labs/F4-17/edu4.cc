// edu4.cc - the F4-17 driver. Each step cites the design note (design_note.txt) row it
// implements; register offsets come from edu_regs.h (observed in F4-16, documentation not
// opened in this build).
#include "edu4.h"
#include "edu_regs.h"
#include "k4.h"
#include <linux/pci_regs.h>

namespace edu4 {

using namespace edu_regs;

const char* to_string(Err e)
{
    switch (e) {
    case Err::Ok: return "ok";
    case Err::NoDevice: return "no device";
    case Err::Gone: return "device gone (all-ones status, or ID no longer as observed)";
    case Err::BadId: return "unexpected ID value";
    case Err::CheckFailed: return "check register did not invert";
    case Err::Timeout: return "timeout";
    case Err::StuckIrq: return "interrupt status did not clear";
    }
    return "?";
}

static uint32_t rd(const Device& d, uint32_t off) { return k4::mmio_read32(d.base + off); }
static void wr(const Device& d, uint32_t off, uint32_t v) { k4::mmio_write32(d.base + off, v); }

bool matches(const pci4::Function& f) { return f.vendor == kVendor && f.device == kDevice; }

Err probe(const pci4::Function& f, Device& d)
{
    if (!matches(f)) return Err::NoDevice;
    d = Device{};
    d.at = f.at;
    d.timeout_ticks = 50000000;                         // design note, "Error handling"
    // Resources: one 32-bit memory BAR (design note, "Resources").
    const pci4::Bar bar = pci4::read_bar(f.at, 0);
    if (bar.io || bar.size != kBar0Size || bar.base == 0) return Err::NoDevice;
    d.base = bar.base;
    // Initialization sequence: memory decoding on, no bus mastering (no DMA in this driver).
    const uint16_t cmd = pci4::read16(f.at, PCI_COMMAND);
    pci4::write32(f.at, PCI_COMMAND, (cmd | PCI_COMMAND_MEMORY) & ~PCI_COMMAND_MASTER);
    const uint32_t id = rd(d, kId);
    if (id == 0xFFFFFFFFu) return Err::Gone;
    if (id != kIdSeen) return Err::BadId;              // a revision we have never observed
    wr(d, kCheck, 0x5A5AA5A5);
    if (rd(d, kCheck) != ~0x5A5AA5A5u) return Err::CheckFailed;
    // Interrupt path check without interrupts: raise, see it, acknowledge, see it clear.
    wr(d, kIrqRaise, 0x1);
    const bool raised = rd(d, kIrqStatus) == 0x1;
    wr(d, kIrqAck, 0x1);
    if (!raised || rd(d, kIrqStatus) != 0) return Err::StuckIrq;
    return Err::Ok;
}

Err compute(Device& d, uint32_t n, uint32_t& result)
{
    wr(d, kCompute, n);
    const uint64_t start = k4::rdtsc();
    for (;;) {
        const uint32_t st = rd(d, kStatus);
        ++d.stats.polls;
        if (st == 0xFFFFFFFFu) { ++d.stats.gone; return Err::Gone; }   // removal on real PCIe
        if (!(st & kStatusBusy)) break;
        if (k4::rdtsc() - start > d.timeout_ticks) { ++d.stats.timeouts; return Err::Timeout; }
    }
    result = rd(d, kCompute);
#ifndef F417_V1
    // "Not busy" is also what a device that stopped answering may return (in QEMU, reads of a
    // BAR whose decoding is off return 0: F4-17 forensic). Confirm the device is still there.
    if (rd(d, kId) != kIdSeen) { ++d.stats.gone; return Err::Gone; }
#endif
    ++d.stats.computes;
    return Err::Ok;
}

void dump(const Device& d)
{
    k4::puts("edu4: regs id=0x"); k4::hex(rd(d, kId), 8);
    k4::puts(" status=0x"); k4::hex(rd(d, kStatus), 8);
    k4::puts(" irq_status=0x"); k4::hex(rd(d, kIrqStatus), 8);
    k4::puts(" | computes "); k4::dec(d.stats.computes);
    k4::puts(" polls "); k4::dec(d.stats.polls);
    k4::puts(" timeouts "); k4::dec(d.stats.timeouts);
    k4::puts(" gone "); k4::dec(d.stats.gone);
    k4::putc('\n');
}

} // namespace edu4
