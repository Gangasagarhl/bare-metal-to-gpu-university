// f440_main.cc - F4-40: start guests on the tiny hypervisor and report every exit.
// Command line: test=hello | triple | invalid | busy | hlt
#include "hv.h"
#include "intr.h"
#include "kio.h"

extern "C" void guest_hello();
extern "C" void guest_triple();
extern "C" void guest_busy();
extern "C" void guest_hlt();

namespace {
Vmcb g_vmcb;
alignas(16) uint8_t g_guest_stack[16384];

// Deliberately invalid guest states, each breaking one rule of the APM's list of VMRUN
// consistency checks (from memory; not opened in this build). R3 shows which ones
// QEMU 8.2.2 enforces.
enum class Break { None, VmrunIntercept, Asid0, EferSvme, Cr0NwWithoutCd };

void run_guest(const char* what, void (*entry)(), Break b = Break::None)
{
    Vcpu v;
    vcpu_init_long_mode(v, &g_vmcb, reinterpret_cast<uint64_t>(entry),
                        reinterpret_cast<uint64_t>(g_guest_stack + sizeof g_guest_stack));
    const char* broken = nullptr;
    switch (b) {
    case Break::None: break;
    case Break::VmrunIntercept:
        g_vmcb.u32(vmcb::INTERCEPT_W4) &= ~w4_bit(SVM_EXIT_VMRUN);
        broken = "VMRUN intercept cleared"; break;
    case Break::Asid0:
        g_vmcb.u32(vmcb::GUEST_ASID) = 0;
        broken = "guest ASID 0 (reserved for the host)"; break;
    case Break::EferSvme:
        g_vmcb.u64(vmcb::EFER) &= ~(1ull << 12);
        broken = "guest EFER.SVME = 0"; break;
    case Break::Cr0NwWithoutCd:
        g_vmcb.u64(vmcb::CR0) = (g_vmcb.u64(vmcb::CR0) | (1ull << 29)) & ~(1ull << 30);
        broken = "guest CR0.NW = 1 with CR0.CD = 0"; break;
    }
    if (broken) {
        kprintf("hv: guest '%s' with an invalid state on purpose: %s\n", what, broken);
    }
    v.trace = arg_num("trace", 0);
    kprintf("hv: starting guest '%s' at rip %lx\n", what, g_vmcb.u64(vmcb::RIP));
    vcpu_run(v, 50000000);
    vcpu_report(v);
}
}

extern "C" [[noreturn]] void kmain(uint32_t magic, uint32_t info)
{
    serial_init();
    cmdline_init(magic, info);
    kprintf("DR404 tiny hypervisor (F4-40); cmdline \"%s\"\n", cmdline());
    idt_init();
    timer_start(100);
    irq_enable();
    host_calibrate_tsc();
    irq_disable();
    kprintf("hv: host TSC %lu Hz (measured against the PIT)\n", g_tsc_hz);
    if (!svm_enable()) {
        qemu_exit(1);
    }
    if (arg_is("test", "hello")) {
        run_guest("hello", guest_hello);
    } else if (arg_is("test", "triple")) {
        run_guest("triple", guest_triple);
        kprintf("hv: host still running; starting a new guest\n");
        run_guest("hello", guest_hello);
    } else if (arg_is("test", "invalid")) {
        run_guest("hello", guest_hello, Break::VmrunIntercept);
        run_guest("hello", guest_hello, Break::Asid0);
        run_guest("hello", guest_hello, Break::EferSvme);
        run_guest("hello", guest_hello, Break::Cr0NwWithoutCd);
        kprintf("hv: host still running; starting the same guest with a valid state\n");
        run_guest("hello", guest_hello);
    } else if (arg_is("test", "busy")) {
        run_guest("busy", guest_busy);
    } else if (arg_is("test", "hlt")) {
        run_guest("hlt", guest_hlt);
    }
    kprintf("hv: done\n");
    qemu_exit(0);
}
