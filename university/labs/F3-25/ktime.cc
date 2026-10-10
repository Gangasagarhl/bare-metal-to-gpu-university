// ktime.cc - F3-25: HPET and PIT drivers, TSC and APIC timer calibration, tickless one-shot timer.
// HPET register offsets, PIT ports and commands, the APIC timer registers and the CPUID bits
// are written from memory of the IA-PC HPET Specification, the 8254 datasheet and the Intel SDM
// Vol. 3; the chapter's unverified box lists every one of them.
#include "ktime.h"
#include <cstdint>
#include "acpi.h"
#include "apic.h"
#include "arch.h"
#include "interrupts.h"
#include "kprint.h"
#include "log.h"

namespace ktime {
namespace {

// ---- HPET ----
constexpr uint32_t kHpetCap = 0x000, kHpetConfig = 0x010, kHpetCounter = 0x0F0;
volatile uint8_t* g_hpet = nullptr;

uint64_t hpet_read64(uint32_t off) { return *reinterpret_cast<volatile uint64_t*>(g_hpet + off); }
void hpet_write64(uint32_t off, uint64_t v) { *reinterpret_cast<volatile uint64_t*>(g_hpet + off) = v; }
uint64_t hpet_counter() { return hpet_read64(kHpetCounter); }

bool hpet_init(uint64_t& hz)
{
    const acpi::Header* t = acpi::find("HPET");
    if (t == nullptr) {
        kprintf("time: no HPET table in ACPI\n");
        return false;
    }
    // the table's base address is a Generic Address Structure at offset 40; its 64-bit address at 44
    const auto* b = reinterpret_cast<const uint8_t*>(t);
    uint64_t phys = *reinterpret_cast<const uint64_t*>(b + 44);
    g_hpet = reinterpret_cast<volatile uint8_t*>(apic::map_mmio(phys, 0x400));
    if (g_hpet == nullptr) {
        return false;
    }
    uint64_t cap = hpet_read64(kHpetCap);
    uint64_t period_fs = cap >> 32;              // femtoseconds per count
    if (period_fs == 0 || period_fs > 100000000) {   // the spec caps the period at 100 ns
        kprintf("time: HPET reports an impossible period %lu fs\n", period_fs);
        return false;
    }
    hz = 1000000000000000ull / period_fs;
    hpet_write64(kHpetConfig, hpet_read64(kHpetConfig) & ~uint64_t{3});   // stop, no legacy routing
    hpet_write64(kHpetCounter, 0);
    hpet_write64(kHpetConfig, hpet_read64(kHpetConfig) | 1);             // ENABLE_CNF: counting
    kprintf("time: HPET at %08lx, capabilities %016lx: period %lu fs (%lu Hz), %lu comparators, %s counter\n",
            phys, cap, period_fs, hz, ((cap >> 8) & 0x1F) + 1, (cap >> 13) & 1 ? "64-bit" : "32-bit");
    return true;
}

// ---- PIT channel 2 as a stopwatch (the fallback reference) ----
constexpr uint64_t kPitHz = 1193182;

// Counts down 'ticks' PIT periods on channel 2 (mode 0) and returns the TSC cycles that passed.
uint64_t pit_window_tsc(uint16_t ticks)
{
    arch::outb(0x61, static_cast<uint8_t>((arch::inb(0x61) & ~0x02) | 0x01));   // gate on, speaker off
    arch::outb(0x43, 0xB0);                    // channel 2, low then high byte, mode 0, binary
    arch::outb(0x42, static_cast<uint8_t>(ticks & 0xFF));
    uint64_t t0 = arch::rdtsc();
    arch::outb(0x42, static_cast<uint8_t>(ticks >> 8));   // counting starts with the high byte
    while ((arch::inb(0x61) & 0x20) == 0) {    // OUT2 goes high at terminal count
    }
    return arch::rdtsc() - t0;
}

// ---- TSC ----
uint64_t g_tsc_hz = 0, g_tsc_base = 0;
uint64_t tsc_counter() { return arch::rdtsc() - g_tsc_base; }

bool tsc_invariant()
{
    uint32_t a, b, c, d;
    arch::cpuid(0x80000000, 0, a, b, c, d);
    if (a < 0x80000007) {
        return false;
    }
    arch::cpuid(0x80000007, 0, a, b, c, d);
    return (d >> 8) & 1;
}

// ---- the chosen clock source ----
ClockSource g_src{"none", nullptr, 1};
uint64_t g_src_base = 0;

// ---- local APIC timer ----
constexpr uint32_t kLvtTimer = 0x320, kInitialCount = 0x380, kCurrentCount = 0x390, kDivide = 0x3E0;
constexpr uint8_t kTimerVector = 0x40;
uint64_t g_apic_hz = 0;                     // after the divide-by-16 setting
TimerQueue<32> g_queue;
volatile uint64_t g_irqs = 0;

void arm()                                  // interrupts are off when this runs
{
    if (g_queue.empty()) {
        apic::write_lapic(kInitialCount, 0);   // a count of 0 stops the timer
        return;
    }
    uint64_t now = now_ns(), next = g_queue.next_deadline();
    uint64_t delta = next > now ? next - now : 1000;   // late already: fire in 1 us
    uint64_t count = ns_to_cycles(delta, g_apic_hz);
    if (count == 0) {
        count = 1;
    }
    if (count > 0xFFFFFFFFull) {
        count = 0xFFFFFFFFull;             // too far away: wake up early and re-arm
    }
    apic::write_lapic(kInitialCount, static_cast<uint32_t>(count));
}

void on_timer(InterruptFrame&)
{
    g_irqs = g_irqs + 1;
    g_queue.expire(now_ns());
    arm();
    apic::eoi();
}

} // namespace

uint64_t now_ns() { return cycles_to_ns(g_src.read() - g_src_base, g_src.hz); }
const ClockSource& source() { return g_src; }
uint64_t timer_interrupts() { return g_irqs; }
uint64_t apic_timer_hz() { return g_apic_hz; }
uint64_t tsc_hz() { return g_tsc_hz; }

bool init()
{
    uint64_t hpet_hz = 0;
    bool have_hpet = hpet_init(hpet_hz);
    g_tsc_base = arch::rdtsc();
    // 1. calibrate the TSC against the HPET, or against the PIT when there is no HPET
    if (have_hpet) {
        uint64_t h0 = hpet_counter(), t0 = arch::rdtsc();
        uint64_t window = hpet_hz / 20;                         // 50 ms
        while (hpet_counter() - h0 < window) {
        }
        uint64_t h1 = hpet_counter(), t1 = arch::rdtsc();
        g_tsc_hz = (t1 - t0) * hpet_hz / (h1 - h0);
        kprintf("time: TSC calibrated against the HPET over %lu HPET counts: %lu Hz\n", h1 - h0, g_tsc_hz);
    } else {
#if defined(B8_PIT_BUG)
        constexpr uint32_t kLong = kPitHz / 10;                 // meant: 100 ms; does not fit in 16 bits
#else
        constexpr uint32_t kLong = kPitHz / 20;                 // 50 ms = 59659 ticks, fits in 16 bits
#endif
        constexpr uint32_t kShort = kPitHz / 100;               // 10 ms = 11931 ticks
        // Two windows: the fixed cost of starting and polling the PIT appears in both and cancels
        // in the difference, which matters under emulation where each port access is slow.
        uint64_t c_short = pit_window_tsc(static_cast<uint16_t>(kShort));
        uint64_t c_long = pit_window_tsc(static_cast<uint16_t>(kLong));
        uint64_t diff_ns = uint64_t{kLong - kShort} * 1000000000ull / kPitHz;
        g_tsc_hz = (c_long - c_short) * 1000000000ull / diff_ns;
        kprintf("time: TSC calibrated against PIT channel 2, windows of %u and %u ticks (%u requested): %lu Hz\n",
                unsigned{static_cast<uint16_t>(kShort)}, unsigned{static_cast<uint16_t>(kLong)}, kLong, g_tsc_hz);
    }
    // 2. pick the clock source: an invariant TSC is best, then the HPET, then a plain TSC
    bool inv = tsc_invariant();
    if (inv || !have_hpet) {
        g_src = ClockSource{"tsc", tsc_counter, g_tsc_hz};
    } else {
        g_src = ClockSource{"hpet", hpet_counter, hpet_hz};
    }
    g_src_base = g_src.read();
    kprintf("time: invariant TSC %s; clock source '%s' at %lu Hz\n", inv ? "yes" : "no", g_src.name, g_src.hz);
    if (!inv && !have_hpet) {
        klog(Level::Warn, "time: TSC without the invariant flag: its rate may change with power states");
    }
    // 3. calibrate the local APIC timer against the clock source (divide by 16, 50 ms)
    apic::write_lapic(kDivide, 0x3);
    apic::write_lapic(kLvtTimer, (1u << 16) | kTimerVector);   // masked while measuring
    uint64_t c0 = now_ns();
    apic::write_lapic(kInitialCount, 0xFFFFFFFF);
    while (now_ns() - c0 < 50000000) {
    }
    uint32_t left = apic::read_lapic(kCurrentCount);
    uint64_t c1 = now_ns();
    g_apic_hz = (0xFFFFFFFFull - left) * 1000000000ull / (c1 - c0);
    apic::write_lapic(kInitialCount, 0);
    kprintf("time: APIC timer (divide by 16) calibrated: %lu Hz\n", g_apic_hz);
    idt::set_handler(kTimerVector, on_timer);
    apic::write_lapic(kLvtTimer, kTimerVector);                  // one-shot mode, unmasked
    return g_apic_hz != 0 && g_tsc_hz != 0;
}

bool add_timer(Timer& t)
{
    uint64_t f = arch::save_flags_cli();
    bool ok = g_queue.add(t);
    arm();
    arch::restore_flags(f);
    return ok;
}

void cancel_timer(Timer& t)
{
    uint64_t f = arch::save_flags_cli();
    g_queue.remove(t);
    arm();
    arch::restore_flags(f);
}

void sleep_ns(uint64_t ns)
{
    volatile bool done = false;
    Timer t;
    t.deadline_ns = now_ns() + ns;
    t.ctx = const_cast<bool*>(&done);
    t.fn = [](Timer& self) { *static_cast<volatile bool*>(self.ctx) = true; };
    add_timer(t);
    for (;;) {
        arch::cli();
        if (done) {
            break;
        }
        arch::sti_hlt();                       // the wake-up cannot slip in between check and halt
    }
    arch::sti();
}

} // namespace ktime
