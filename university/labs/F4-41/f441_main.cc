// f441_main.cc - F4-41: run the F4-39 kernel, unchanged, as a guest with nested paging.
// Command line: the part after "guest:" is handed to the guest as its own command line,
// for example  test=run guest:test=idle idle=hlt seconds=2
#include "hv.h"
#include "intr.h"
#include "kio.h"
#include "npt.h"
#include "vintr.h"

extern "C" const uint8_t guest_image_start[];
extern "C" const uint8_t guest_image_end[];

namespace {
Vmcb g_vmcb;

const char* guest_cmdline()
{
    const char* c = cmdline();
    for (const char* p = c; *p; ++p) {
        if (memcmp(p, "guest:", 6) == 0) { return p + 6; }
    }
    return "test=detect";
}
}

extern "C" [[noreturn]] void kmain(uint32_t magic, uint32_t info)
{
    serial_init();
    cmdline_init(magic, info);
    kprintf("DR404 tiny hypervisor with nested paging (F4-41); cmdline \"%s\"\n", cmdline());
    idt_init();
    timer_start(100);
    irq_enable();
    host_calibrate_tsc();
    irq_disable();
    kprintf("hv: host TSC %lu Hz (measured against the PIT)\n", g_tsc_hz);
    if (!svm_enable()) {
        qemu_exit(1);
    }
    if (!(cpuid(0x8000000A).edx & 1)) {               // APM: Fn8000_000A EDX bit 0 = NP
        kprintf("hv: no nested paging on this CPU\n");
        qemu_exit(1);
    }
    uint64_t ncr3 = npt_build();
    kprintf("hv: nested page tables at %lx map guest-physical 0..%lx to host %lx\n", ncr3,
            GUEST_RAM_SIZE - 1, reinterpret_cast<uint64_t>(guest_ram()));
    GuestImage img;
    uint64_t size = static_cast<uint64_t>(guest_image_end - guest_image_start);
    if (!load_guest_elf(guest_image_start, size, img)) {
        qemu_exit(1);
    }
    Vcpu v;
    vcpu_init_guest_kernel(v, &g_vmcb, img, guest_cmdline(), ncr3);
    vintr_attach(v);
    kprintf("hv: starting the F4-39 kernel at gpa %lx with \"%s\"\n", g_vmcb.u64(vmcb::RIP),
            guest_cmdline());
    vcpu_run(v, 50000000);
    vcpu_report(v);
    kprintf("hv: timer interrupts injected: %lu\n", vintr_injected());
    bool ok = (v.stop == Stop::GuestExit && v.guest_exit_code == 0) ||
              (arg_is("expect", "npf") && v.stop == Stop::NestedFault);
    kprintf("hv: done: %s\n", ok ? "PASS" : "FAIL");
    qemu_exit(ok ? 0 : 1);
}
