// npt_probe.cc - does QEMU's SVM model apply nested paging when the guest has paging off?
// The nested table maps guest-physical 0..4 GiB to the same host addresses, except one
// 2 MiB hole at 48 MiB. A 32-bit guest reads the hole, first with paging off, then with
// paging on (its own 4 MiB pages, identity). The hole must cause a nested page fault
// (NPF) in both cases if nested paging translates every guest-physical address.
#include "hv.h"
#include "intr.h"
#include "kio.h"

namespace {
Vmcb g_vmcb;
alignas(4096) uint64_t g_pml4[512], g_pdpt[512], g_pd[4][512];   // nested tables
alignas(4096) uint32_t g_guest_pd[1024];                         // guest's own table
constexpr uint64_t HOLE = 0x3000000;
// 32-bit guest code: mov eax, [HOLE]; xor eax, eax; mov ebx, 7; vmmcall; jmp $
alignas(16) uint8_t g_code[] = {0xA1, 0x00, 0x00, 0x00, 0x03, 0x31, 0xC0, 0xBB, 0x07, 0x00,
                                0x00, 0x00, 0x0F, 0x01, 0xD9, 0xEB, 0xFE};

bool report_npf(Vcpu& v, uint64_t code)
{
    if (code != SVM_EXIT_NPF) { return false; }
    kprintf("  NPF: guest-physical %lx, error code %lx\n", v.vmcb->u64(vmcb::EXITINFO2),
            v.vmcb->u64(vmcb::EXITINFO1));
    v.stop = Stop::NestedFault;
    return true;
}

void probe(bool guest_paging)
{
    Vcpu v;
    vcpu_init_long_mode(v, &g_vmcb, reinterpret_cast<uint64_t>(g_code), 0x8000);
    g_vmcb.seg(vmcb::CS, 0x08, 0x0C9B, 0xFFFFFFFF, 0);   // 32-bit protected mode
    g_vmcb.seg(vmcb::DS, 0x10, 0x0C93, 0xFFFFFFFF, 0);
    g_vmcb.seg(vmcb::SS, 0x10, 0x0C93, 0xFFFFFFFF, 0);
    g_vmcb.u64(vmcb::EFER) = 1ull << 12;                 // SVME only
    g_vmcb.u64(vmcb::CR0) = guest_paging ? 0x80000011 : 0x11;
    g_vmcb.u64(vmcb::CR4) = guest_paging ? 0x10 : 0;    // PSE: 4 MiB pages
    g_vmcb.u64(vmcb::CR3) = guest_paging ? reinterpret_cast<uint64_t>(g_guest_pd) : 0;
    g_vmcb.u64(vmcb::NP_ENABLE) = 1;
    g_vmcb.u64(vmcb::N_CR3) = reinterpret_cast<uint64_t>(g_pml4);
    v.exit_hook = report_npf;
    kprintf("guest paging %s: reading guest-physical %lx (a hole in the nested table)\n",
            guest_paging ? "ON " : "OFF", HOLE);
    vcpu_run(v, 100);
    kprintf("  result: %s\n", v.stop == Stop::NestedFault ? "nested page fault (translated)"
                              : v.stop == Stop::GuestExit ? "the read succeeded: NOT translated"
                                                          : "other");
}
}

extern "C" [[noreturn]] void kmain(uint32_t magic, uint32_t info)
{
    serial_init();
    cmdline_init(magic, info);
    kprintf("DR404 nested-paging probe (F4-41)\n");
    idt_init();
    timer_start(100);
    irq_enable();
    host_calibrate_tsc();
    irq_disable();
    if (!svm_enable()) { qemu_exit(1); }
    for (uint64_t j = 0; j < 4; ++j) {
        g_pdpt[j] = reinterpret_cast<uint64_t>(g_pd[j]) | 7;            // P | RW | US
        for (uint64_t i = 0; i < 512; ++i) {
            g_pd[j][i] = ((j << 30) + (i << 21)) | 0x87;                // + PS: 2 MiB
        }
    }
    g_pml4[0] = reinterpret_cast<uint64_t>(g_pdpt) | 7;
    g_pd[0][HOLE >> 21] = 0;                                            // the hole
    for (uint32_t i = 0; i < 1024; ++i) {
        g_guest_pd[i] = (i << 22) | 0x87;                               // identity, 4 MiB
    }
    probe(false);
    probe(true);
    qemu_exit(0);
}
