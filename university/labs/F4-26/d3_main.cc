// d3_main.cc - F4-26: milestone D3's arch pieces on QEMU virt: every CPU of the devicetree
// started through PSCI CPU_ON, per-CPU data through TPIDR_EL1, a lock tested by all CPUs at
// once, an SGI to every CPU, a sector read through virtio-mmio, and PSCI SYSTEM_OFF.
#include <atomic>
#include <cstdint>
#include "arch.h"
#include "fdt.h"
#include "gic.h"
#include "kprint.h"
#include "mmu.h"
#include "pl011.h"
#include "psci.h"
#include "spinlock.h"
#include "trap.h"
#include "virtio_mmio.h"

extern "C" char secondary_entry[];

namespace {

constexpr int kMaxCpus = 8;
uint32_t g_rounds = 200000;           // "rounds=N" on the kernel command line changes it
constexpr uint32_t kIpiSgi = 1;

struct PerCpu {
    uint64_t index;
    uint64_t mpidr;
    std::atomic<uint32_t> ipis{0};
};
PerCpu g_cpu[kMaxCpus];
int g_ncpus = 0;
std::atomic<int> g_online{0};
std::atomic<int> g_start{0};          // release all CPUs into the lock test together
std::atomic<int> g_done{0};
TicketLock g_lock;
uint64_t g_locked_counter = 0;        // protected by g_lock
std::atomic<uint64_t> g_racy_counter{0};   // updated with a separate load and store: racy on purpose
TicketLock g_print_lock;

PerCpu& this_cpu() { return *reinterpret_cast<PerCpu*>(READ_SYSREG(tpidr_el1)); }

// The kernel command line arrives as /chosen/bootargs (QEMU: -append "..."). Only "rounds=N".
void parse_bootargs(const fdt::Blob& dt)
{
    fdt::Node chosen;
    fdt::Prop p;
    if (!dt.find_path("/chosen", &chosen) || !dt.get_prop(chosen, "bootargs", &p)) {
        return;
    }
    const char* s = reinterpret_cast<const char*>(p.data);
    kprintf("bootargs: \"%s\"\n", s);
    for (; *s != '\0'; ++s) {
        if (s[0] == 'r' && s[1] == 'o' && s[2] == 'u' && s[3] == 'n' && s[4] == 'd' && s[5] == 's' && s[6] == '=') {
            uint32_t v = 0;
            for (s += 7; *s >= '0' && *s <= '9'; ++s) {
                v = v * 10 + static_cast<uint32_t>(*s - '0');
            }
            g_rounds = v;
            return;
        }
    }
}

void irq_handler(TrapFrame*)
{
    uint32_t id = gic::ack();
    if (id == kIpiSgi) {
        this_cpu().ipis.fetch_add(1, std::memory_order_relaxed);
    }
    if (id != gic::kSpurious) {
        gic::eoi(id);
    }
}

void lock_test(uint64_t)
{
    while (g_start.load(std::memory_order_acquire) == 0) {
        arch::cpu_relax();
    }
    for (uint32_t i = 0; i < g_rounds; ++i) {
        g_lock.lock();
        ++g_locked_counter;
        g_lock.unlock();
        // read-modify-write as two separate steps: another CPU can slip in between
        g_racy_counter.store(g_racy_counter.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    }
    g_done.fetch_add(1, std::memory_order_acq_rel);
}

void cpu_setup(uint64_t index)
{
    WRITE_SYSREG(tpidr_el1, reinterpret_cast<uint64_t>(&g_cpu[index]));   // per-CPU data pointer
    gic::init_cpu();
    gic::enable_ppi(kIpiSgi, 0x80);   // SGIs 0-15 are enabled through the same register as PPIs
    trap::set_irq_handler(irq_handler);
    arch::sti();
}

} // namespace

extern "C" [[noreturn]] void secondary_main(uint64_t index, uint64_t entry_el)
{
    mmu::enable_this_cpu();           // same tables as the boot CPU, before touching shared data
    cpu_setup(index);
    g_print_lock.lock();
    kprintf("cpu %lu online: MPIDR_EL1 0x%lx, entered at EL%lu, TPIDR_EL1 -> g_cpu[%lu]\n", index,
            READ_SYSREG(mpidr_el1), entry_el, this_cpu().index);
    g_print_lock.unlock();
    g_online.fetch_add(1, std::memory_order_release);
    lock_test(index);
    for (;;) {
        arch::wfi();                  // wait for interrupts (the SGI test)
    }
}

extern "C" void kmain(const void* dtb, uint64_t entry_el)
{
    fdt::Blob dt;
    fdt::Node uart;
    uint64_t base = 0, size = 0;
    if (!dt.init(dtb) || !dt.find_compatible("arm,pl011", &uart) || !dt.reg(uart, 0, &base, &size)) {
        arch::halt_forever();
    }
    pl011::set_base(base);
    psci::init(dt);
    kprintf("D3: boot CPU entered at EL%lu; PSCI through %s\n", entry_el, psci::method());
    parse_bootargs(dt);
    if (gic::probe(dt) != gic::Version::V3) {
        kprintf("GICv3 required\n");
        arch::qemu_exit(arch::kExitFail);
    }
    fdt::Node mem;
    uint64_t ram = 0, ram_size = 0;
    dt.find_path("/memory", &mem);
    dt.reg(mem, 0, &ram, &ram_size);
    mmu::init(ram, ram_size);
    gic::init_distributor();

    // CPUs from the devicetree: each cpu node's reg is its MPIDR affinity value.
    dt.for_each_node([&](const fdt::Node& n) {
        fdt::Prop p;
        uint64_t mpidr = 0, unused = 0;
        if (g_ncpus < kMaxCpus && dt.get_prop(n, "device_type", &p) && fdt::streq(reinterpret_cast<const char*>(p.data), "cpu") &&
            dt.reg(n, 0, &mpidr, &unused)) {
            g_cpu[g_ncpus].index = static_cast<uint64_t>(g_ncpus);
            g_cpu[g_ncpus].mpidr = mpidr;
            ++g_ncpus;
        }
        return true;
    });
    uint64_t my_mpidr = READ_SYSREG(mpidr_el1) & 0xff00ffffffull;
    kprintf("devicetree lists %d CPU(s); boot CPU MPIDR 0x%lx\n", g_ncpus, my_mpidr);
    cpu_setup(0);
    g_online.store(1);

    for (int i = 1; i < g_ncpus; ++i) {
        int64_t r = psci::call(psci::kCpuOn64, g_cpu[i].mpidr, reinterpret_cast<uint64_t>(secondary_entry),
                               static_cast<uint64_t>(i));
        g_print_lock.lock();
        kprintf("PSCI CPU_ON(target 0x%lx, entry 0x%lx, context %d) = %ld\n", g_cpu[i].mpidr,
                reinterpret_cast<uint64_t>(secondary_entry), i, r);
        g_print_lock.unlock();
    }
    while (g_online.load(std::memory_order_acquire) < g_ncpus) {
        arch::cpu_relax();
    }
    if (g_ncpus > 1) {
        int64_t again = psci::call(psci::kCpuOn64, g_cpu[1].mpidr, reinterpret_cast<uint64_t>(secondary_entry), 1);
        kprintf("PSCI CPU_ON for a CPU that is already on = %ld\n", again);
    }
    for (int i = 0; i < g_ncpus; ++i) {
        kprintf("PSCI AFFINITY_INFO(0x%lx) = %ld\n", g_cpu[i].mpidr, psci::call(psci::kAffinityInfo64, g_cpu[i].mpidr, 0));
    }

    // Lock test: all CPUs at once
    g_start.store(1, std::memory_order_release);
    lock_test(0);
    while (g_done.load(std::memory_order_acquire) < g_ncpus) {
        arch::cpu_relax();
    }
    uint64_t want = uint64_t{g_rounds} * static_cast<uint64_t>(g_ncpus);
    kprintf("lock test: %d CPUs x %u rounds: locked counter %lu (expected %lu), racy counter %lu\n", g_ncpus,
            g_rounds, g_locked_counter, want, g_racy_counter.load());
    bool lock_ok = g_locked_counter == want;

    // SGI to every other CPU, through the GIC's CPU interface
    for (int i = 1; i < g_ncpus; ++i) {
        gic::send_sgi(kIpiSgi, g_cpu[i].mpidr);
    }
    bool ipi_ok = true;
    for (int i = 1; i < g_ncpus; ++i) {
        for (int spin = 0; spin < 10000000 && g_cpu[i].ipis.load() == 0; ++spin) {
            arch::cpu_relax();
        }
        ipi_ok = ipi_ok && g_cpu[i].ipis.load() == 1;
    }
    kprintf("SGI %u: every other CPU took exactly one: %s\n", kIpiSgi, ipi_ok ? "yes" : "NO");

    // virtio-mmio block device: sector 0 of the disk image run.sh made
    bool disk_ok = false;
    uint64_t blk = vmmio::scan(dt, 2);
    if (blk != 0 && vmmio::blk_init(blk)) {
        alignas(16) static uint8_t sector[512];
        if (vmmio::blk_read(0, sector)) {
            sector[63] = 0;
            kprintf("sector 0 begins: \"%s\"\n", reinterpret_cast<const char*>(sector));
            disk_ok = sector[0] == 'D';
        }
    }
    if (lock_ok && ipi_ok && disk_ok) {
        kprintf("D3 ok\n");
        arch::qemu_exit(arch::kExitPass);
    }
    kprintf("D3 FAILED\n");
    arch::qemu_exit(arch::kExitFail);
}
