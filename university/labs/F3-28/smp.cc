// smp.cc - start the application processors (milestone B11) and TLB shootdown.
// MP initialization: INIT IPI, wait 10 ms, start-up IPI (SIPI), wait 200 us, second
// SIPI; the delays and the order follow the MP initialization example in the Intel SDM
// Vol. 3, "Multiple-Processor Management" (title only; see the unverified box).
#include "hooks.h"
#include "thread.h"

extern "C" uint8_t tramp_start[], tramp_end[], tramp_cr3[], tramp_stack[], tramp_entry[],
    tramp_cpu[];

namespace k {

int acpi_list_cpus(uint32_t* apic_ids, int max, bool verbose);

namespace {
constexpr uint64_t kTrampoline = 0x8000;   // SIPI vector 0x08 = page 0x8000
alignas(16) uint8_t ap_boot_stacks[kMaxCpus][16384];
int madt_cpus = 0;

volatile uint32_t* lapic() { return reinterpret_cast<volatile uint32_t*>(rdmsr(0x1B) & 0xFFFFF000); }

void send_icr(uint32_t apic_id, uint32_t low)
{
    while (lapic()[0x300 / 4] & (1u << 12)) {   // wait until the previous IPI was sent
        cpu_relax();
    }
    lapic()[0x310 / 4] = apic_id << 24;   // destination (ICR high)
    lapic()[0x300 / 4] = low;             // writing the low half sends the IPI
}

template <typename T> void patch(uint8_t* field, T value)   // write a trampoline variable
{
    *reinterpret_cast<T*>(kTrampoline + (field - tramp_start)) = value;
}
}  // namespace

extern "C" [[noreturn]] void ap_entry(Cpu* c)
{
    cpu_setup(*c, c->id, c->apic_id);   // own GDT, TSS and GS base; the shared IDT
    lapic_init_cpu();
    char name[8] = "idle";               // "idle" + CPU number
    int pos = 4;
    if (c->id >= 10) {
        name[pos++] = char('0' + c->id / 10);
    }
    name[pos] = char('0' + c->id % 10);
    sched_init_cpu(*c, name);            // this code becomes the CPU's idle thread
    syscall_init_cpu();
    lapic_timer_start(kTickHz);
    __atomic_store_n(&c->online, true, __ATOMIC_RELEASE);
    idle_forever();
}

void smp_start_aps()
{
    uint32_t ids[kMaxCpus];
    bool verbose = boot_arg("madt") != nullptr;
    madt_cpus = acpi_list_cpus(ids, kMaxCpus, verbose);
    uint32_t bsp = lapic_id();
    memcpy(reinterpret_cast<void*>(kTrampoline), tramp_start, size_t(tramp_end - tramp_start));
    int next = 1;
    for (int i = 0; i < madt_cpus; ++i) {
        if (ids[i] == bsp) {
            continue;
        }
        Cpu& c = cpu_by_index(next);
        c.id = next;
        c.apic_id = ids[i];
        patch<uint64_t>(tramp_cr3, read_cr3());
        patch<uint64_t>(tramp_stack,
                        reinterpret_cast<uint64_t>(ap_boot_stacks[next] + sizeof(ap_boot_stacks[next])));
        patch<uint64_t>(tramp_entry, reinterpret_cast<uint64_t>(ap_entry));
        patch<uint64_t>(tramp_cpu, reinterpret_cast<uint64_t>(&c));
        send_icr(ids[i], 0x4500);            // INIT, level assert
        pit_wait_us(10000);
        for (int sipi = 0; sipi < 2; ++sipi) {
            send_icr(ids[i], 0x4600 | uint32_t(kTrampoline >> 12));   // start-up IPI
            pit_wait_us(200);
        }
        for (int ms = 0; ms < 1000 && !__atomic_load_n(&c.online, __ATOMIC_ACQUIRE); ++ms) {
            pit_wait_us(1000);
        }
        if (!c.online) {
            kprintf("smp: APIC id %u did not come online\n", ids[i]);
            continue;
        }
        ++next;
    }
    if (madt_cpus > 1 || verbose) {
        kprintf("smp: %d CPUs online, the MADT lists %d enabled processors\n", cpu_count(),
                madt_cpus);
    }
}

int smp_madt_cpus() { return madt_cpus; }

// ---------------------------------------------------------------- TLB shootdown
namespace {
Spinlock shoot_lock{"tlb-shootdown"};
volatile uint64_t shoot_va = 0, shoot_pages = 0;
std::atomic<int> shoot_pending{0};
std::atomic<uint64_t> shootdowns{0};
}  // namespace

bool tlb_shootdown_enabled = true;   // a test turns it off to show what goes wrong

void tlb_shootdown(uint64_t va, uint64_t pages)
{
    for (uint64_t i = 0; i < pages; ++i) {
        invlpg(va + i * kPage);
    }
    int others = 0;
    for (int i = 0; i < cpu_count(); ++i) {
        others += cpu_by_index(i).online && &cpu_by_index(i) != &this_cpu();
    }
    if (others == 0 || !tlb_shootdown_enabled) {
        return;
    }
    // Interrupts must be on: another CPU may be waiting for *our* answer right now.
    KASSERT(irqs_enabled());
    shoot_lock.lock();   // keeps interrupts enabled (no handler takes this lock)
    shoot_va = va;
    shoot_pages = pages;
    shoot_pending.store(others, std::memory_order_release);
    for (int i = 0; i < cpu_count(); ++i) {
        Cpu& c = cpu_by_index(i);
        if (c.online && &c != &this_cpu()) {
            lapic_send_ipi(c.apic_id, kVecTlb);
        }
    }
    uint64_t start = rdtsc();
    while (shoot_pending.load(std::memory_order_acquire) > 0) {
        cpu_relax();
        if (rdtsc() - start > 2000 * tsc_per_ms()) {
            panic("TLB shootdown: %d CPUs did not answer", shoot_pending.load());
        }
    }
    shootdowns.fetch_add(1);
    shoot_lock.unlock();
}

void tlb_ipi_handler()
{
    for (uint64_t i = 0; i < shoot_pages; ++i) {
        invlpg(shoot_va + i * kPage);
    }
    shoot_pending.fetch_sub(1, std::memory_order_release);
}

uint64_t tlb_shootdown_count() { return shootdowns.load(); }

}  // namespace k
