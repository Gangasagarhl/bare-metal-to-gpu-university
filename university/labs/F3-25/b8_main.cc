// b8_main.cc - F3-25 test kernel for milestone B8 (timers and time keeping).
//   (none)          clock source, calibrations, timer queue, a 10 s sleep (timed by the host too),
//                   10 million monotonic clock reads, timestamped log lines
//   -DB8_PIT_BUG    the forensic kernel (run with hpet=off): a PIT window that does not fit 16 bits
#include <cstdint>
#include "acpi.h"
#include "apic.h"
#include "arch.h"
#include "gdt.h"
#include "interrupts.h"
#include "kprint.h"
#include "ktime.h"
#include "log.h"
#include "madt.h"
#include "multiboot.h"
#include "paging.h"
#include "panic.h"
#include "pmm.h"
#include "serial.h"

namespace {

madt::Info g_madt;
volatile uint64_t g_ticks = 0;
int g_order[3];
volatile int g_fired = 0;

void queue_test()
{
    Timer t[3];
    const uint64_t delay_ms[3] = {30, 10, 20};          // added out of order on purpose
    uint64_t start = ktime::now_ns();
    for (int i = 0; i < 3; ++i) {
        t[i].deadline_ns = start + delay_ms[i] * 1000000;
        t[i].ctx = reinterpret_cast<void*>(static_cast<uintptr_t>(i));
        t[i].fn = [](Timer& self) {
            g_order[g_fired] = static_cast<int>(reinterpret_cast<uintptr_t>(self.ctx));
            g_fired = g_fired + 1;
        };
        KASSERT(ktime::add_timer(t[i]));
    }
    while (g_fired < 3) {
        arch::hlt();
    }
    kprintf("timer queue: added 30, 10, 20 ms; fired in the order %lu, %lu, %lu ms\n", delay_ms[g_order[0]],
            delay_ms[g_order[1]], delay_ms[g_order[2]]);
    KASSERT(delay_ms[g_order[0]] == 10 && delay_ms[g_order[1]] == 20 && delay_ms[g_order[2]] == 30);
}

void short_sleeps()
{
    constexpr uint64_t kMs[3] = {1, 10, 100};
    for (uint64_t ms : kMs) {
        uint64_t a = ktime::now_ns();
        ktime::sleep_ns(ms * 1000000);
        uint64_t took = ktime::now_ns() - a;
        kprintf("sleep %3lu ms: took %lu us (late by %lu us)\n", ms, took / 1000, (took - ms * 1000000) / 1000);
        KASSERT(took >= ms * 1000000);                   // never early
    }
}

void monotonic_test()
{
    constexpr uint64_t kReads = 10000000;
    uint64_t prev = ktime::now_ns(), backwards = 0, max_step = 0, equal = 0;
    uint64_t a = prev;
    for (uint64_t i = 0; i < kReads; ++i) {
        uint64_t t = ktime::now_ns();
        if (t < prev) {
            ++backwards;
        } else if (t == prev) {
            ++equal;
        } else if (t - prev > max_step) {
            max_step = t - prev;
        }
        prev = t;
    }
    kprintf("monotonic: %lu reads in %lu ms, %lu went backwards, %lu repeated the previous value, "
            "largest step %lu us\n", kReads, (prev - a) / 1000000, backwards, equal, max_step / 1000);
    KASSERT(backwards == 0);
}

} // namespace

extern "C" void kmain(uint32_t magic, uint32_t info_phys)
{
    serial::init();
    KASSERT(magic == mb::kBootMagic);
    gdt::init(true);
    idt::init(true);
    pmm::init(static_cast<const mb::Info*>(pmm::phys_to_virt(info_phys)), true);
    KASSERT(paging::build_kernel_space(info_phys));
    log_init(false);
    KASSERT(acpi::init());
    const acpi::Header* m = acpi::find("APIC");
    KASSERT(m != nullptr && madt::parse(reinterpret_cast<const uint8_t*>(m), m->length, g_madt));
    apic::disable_8259();
    KASSERT(apic::init_local(g_madt.lapic_address));
    KASSERT(ktime::init());
    log_set_clock(ktime::now_ns);
    arch::sti();
    klog(Level::Info, "OS302 kernel, chapter F3-25 build: log lines now carry clock-source time");

    Timer tick;                                          // a 100 Hz "scheduler tick" from the queue
    tick.period_ns = 10000000;
    tick.deadline_ns = ktime::now_ns() + tick.period_ns;
    tick.fn = [](Timer&) { g_ticks = g_ticks + 1; };
    KASSERT(ktime::add_timer(tick));

    queue_test();
    short_sleeps();
    uint64_t irq0 = ktime::timer_interrupts(), tick0 = g_ticks;
    klog(Level::Info, "sleep: start (10 s)");
    uint64_t a = ktime::now_ns();
    ktime::sleep_ns(10000000000ull);
    uint64_t took = ktime::now_ns() - a;
    klog(Level::Info, "sleep: end, the kernel clock measured %lu.%06lu s", took / 1000000000, (took / 1000) % 1000000);
    kprintf("during the sleep: %lu scheduler ticks (100 Hz), %lu APIC timer interrupts\n", g_ticks - tick0,
            ktime::timer_interrupts() - irq0);
    ktime::cancel_timer(tick);
    monotonic_test();
    klog(Level::Info, "B8 ok: clock source '%s', TSC %lu Hz, APIC timer %lu Hz", ktime::source().name, ktime::tsc_hz(),
         ktime::apic_timer_hz());
    arch::qemu_exit(arch::kExitPass);
}
