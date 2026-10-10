// mp1_a64.cc - MP1 starter: the AArch64 port of the regression kernel.
// Boot path reused unchanged from F4-24 (boot.S, vectors.S, trap.cc, pl011.cc, psci.cc),
// the devicetree reader from F4-23 (fdt.h) and the virtio-mmio slot scan from F4-26
// (virtio_mmio.cc). New here: the counter, the CPU count from /cpus, the root-device wait.
#include <cstdint>
#include "arch.h"
#include "fdt.h"
#include "kprint.h"
#include "mp1_core.h"
#include "pl011.h"
#include "psci.h"
#include "virtio_mmio.h"

namespace {

fdt::Blob g_dt;

// The generic timer's virtual counter and its frequency (Arm ARM, generic timer chapter;
// title only, pending verification, as in F4-25). Readable at EL1.
uint64_t now() { return READ_SYSREG(cntvct_el0); }

int count_cpus_dt()
{
    int n = 0;
    g_dt.for_each_node([&](const fdt::Node& node) {
        fdt::Prop p;
        if (g_dt.get_prop(node, "device_type", &p) &&
            fdt::streq(reinterpret_cast<const char*>(p.data), "cpu")) {
            ++n;
        }
        return true;
    });
    return n;
}

// A root device is a virtio block device (device ID 2) in one of the devicetree's
// virtio,mmio slots. On a board the SD card or USB disk can appear late, so the policy
// "rootwait=MS" rescans until it appears or the time is up.
bool find_root_device(uint64_t wait_ms, char* desc, size_t n)
{
    uint64_t hz = READ_SYSREG(cntfrq_el0);
    uint64_t start = now();
    uint64_t deadline = start + wait_ms * (hz / 1000);
    uint64_t next_report = start;
    for (;;) {
        uint64_t base = vmmio::scan(g_dt, 2);
        if (base != 0) {
            ksnprintf(desc, static_cast<int>(n), "virtio-blk in the slot at 0x%lx",
                      static_cast<unsigned long>(base));
            return true;
        }
        uint64_t t = now();
        if (t >= deadline) {
            ksnprintf(desc, static_cast<int>(n), "no block device after waiting %lu ms",
                      static_cast<unsigned long>(wait_ms));
            return false;
        }
        if (t >= next_report) {
            kprintf("storage: waiting for a root device (rootwait=%lu ms)\n",
                    static_cast<unsigned long>(wait_ms));
            next_report = t + hz;            // at most one message per second
        }
        while (now() < t + hz / 2 && now() < deadline) {   // rescan twice a second
            arch::cpu_relax();
        }
    }
}

} // namespace

extern "C" void kmain(const void* dtb, uint64_t entry_el)
{
    uint64_t t0 = now();
    fdt::Node uart, chosen;
    fdt::Prop p;
    uint64_t base = 0, size = 0;
    if (!g_dt.init(dtb) || !g_dt.find_compatible("arm,pl011", &uart) ||
        !g_dt.reg(uart, 0, &base, &size)) {
        arch::halt_forever();               // no devicetree, no console: nothing can be reported
    }
    pl011::set_base(base);
    psci::init(g_dt);
    kprintf("MP1 starter kernel, AArch64 port (F4-24 boot path), entered at EL%lu\n",
            static_cast<unsigned long>(entry_el));
    const char* args = "";
    if (g_dt.find_path("/chosen", &chosen) && g_dt.get_prop(chosen, "bootargs", &p)) {
        args = reinterpret_cast<const char*>(p.data);
    }
    mp1::Platform plat{
        arch::kName, "devicetree", dtb, arch::kPageSize, args, t0, READ_SYSREG(cntfrq_el0), now,
        count_cpus_dt, find_root_device,
    };
    int failures = mp1::run(plat);
    arch::qemu_exit(failures == 0 ? arch::kExitPass : arch::kExitFail);
}
