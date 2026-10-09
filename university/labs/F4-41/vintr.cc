// vintr.cc - the guest's interrupt controller (a small 8259 model), timer interrupts
// injected through EVENTINJ, sleeping on HLT, and nested-page-fault reports.
// EVENTINJ format (vector in bits 7-0, type in 10-8 with 0 = external interrupt, valid in
// bit 31) and the NPF EXITINFO meanings are from memory of the AMD64 APM Vol. 2 (not
// opened in this build); the run proves them on QEMU 8.2.2 only.
#include "vintr.h"
#include "kio.h"

namespace {
struct VirtualPic {
    uint8_t base = 0x08;        // vector of IRQ 0 (the BIOS default until the guest remaps)
    uint8_t mask = 0xFF;
    int icw_step = 0;           // initialisation words still expected on the data port
    bool in_service = false;    // IRQ 0 delivered, EOI not yet seen
};
VirtualPic g_pic;
uint64_t g_next_tick;           // host TSC of the next virtual timer interrupt
uint64_t g_injected;

bool pic_io(Vcpu&, uint16_t port, bool in, uint32_t& val)
{
    if (port == 0x20) {
        if (in) { val = 0; return true; }
        if (val & 0x10) { g_pic.icw_step = 1; }              // ICW1: start initialisation
        else if ((val & 0xE0) == 0x20) { g_pic.in_service = false; }   // EOI
        return true;
    }
    if (port == 0x21) {
        if (in) { val = g_pic.mask; return true; }
        if (g_pic.icw_step == 1) { g_pic.base = static_cast<uint8_t>(val & 0xF8); g_pic.icw_step = 2; }
        else if (g_pic.icw_step == 2) { g_pic.icw_step = 3; }          // ICW3
        else if (g_pic.icw_step == 3) { g_pic.icw_step = 0; }          // ICW4
        else { g_pic.mask = static_cast<uint8_t>(val); }              // OCW1: mask
        return true;
    }
    if (port == 0xA0 || port == 0xA1) {      // the slave PIC: accepted, nothing behind it
        val = 0;
        return true;
    }
    return false;
}

bool timer_due(Vcpu& v)
{
    return v.pit.reload != 0 && !(g_pic.mask & 1) && rdtsc() >= g_next_tick;
}

void inject_if_due(Vcpu& v)
{
    if (v.pit.reload != 0 && g_next_tick == 0) {
        g_next_tick = pit_next_tick_tsc(v.pit);
    }
    bool guest_if = v.vmcb->u64(vmcb::RFLAGS) & 0x200;
    if (!timer_due(v) || !guest_if || g_pic.in_service) {
        return;
    }
    v.vmcb->u64(vmcb::EVENTINJ) = g_pic.base | (0u << 8) | (1u << 31);   // IRQ 0, external
    g_pic.in_service = true;
    ++g_injected;
    g_next_tick = pit_next_tick_tsc(v.pit);   // ticks missed meanwhile are dropped
}

bool sleep_until_tick(Vcpu& v)
{
    bool guest_if = v.vmcb->u64(vmcb::RFLAGS) & 0x200;
    if (!guest_if || v.pit.reload == 0 || (g_pic.mask & 1)) {
        return false;                       // nothing could wake it: F4-40's rule decides
    }
    if (g_next_tick == 0) {
        g_next_tick = pit_next_tick_tsc(v.pit);
    }
    wait_until_tsc(g_next_tick);            // the host halts; the entry hook injects
    return true;
}

bool npf(Vcpu& v, uint64_t code)
{
    if (code != SVM_EXIT_NPF) {
        return false;
    }
    uint64_t err = v.vmcb->u64(vmcb::EXITINFO1);
    uint64_t gpa = v.vmcb->u64(vmcb::EXITINFO2);
    kprintf("hv: nested page fault: guest-physical address %lx, error code %lx (%s, %s%s)\n",
            gpa, err, (err & 1) ? "present" : "not present", (err & 2) ? "write" : "read",
            (err & (1ull << 33)) ? ", during the guest's page-table walk" : "");
    kprintf("hv: guest rip %lx; no memory is mapped there; guest stopped\n",
            v.vmcb->u64(vmcb::RIP));
    v.stop = Stop::NestedFault;
    return true;
}
}

void vintr_attach(Vcpu& v)
{
    g_pic = VirtualPic{};
    g_next_tick = 0;
    g_injected = 0;
    v.io_hook = pic_io;
    v.entry_hook = inject_if_due;
    v.halt_hook = sleep_until_tick;
    v.exit_hook = npf;
}

uint64_t vintr_injected() { return g_injected; }
