// f422_main.cc - F4-22 idle kernel: a timer interrupt 100 times a second and an idle loop that
// either halts (hlt: the CPU stops until the next interrupt) or polls (spins reading the tick
// counter). The choice is made by a small idle governor: halt when the predicted idle time is
// longer than a threshold in microseconds, poll when it is shorter.
// Build variants:  -DF422_FORCE_POLL  always poll (the comparison case)
//                  -DF422_UNIT_BUG    the forensic case: the governor's tick-to-microsecond
//                                     conversion is wrong
// Hardware programming written from memory (chapter's unverified box): 8259A PIC remap to
// vectors 0x20..0x2F, 8254 PIT channel 0 in mode 2 with input clock 1193182 Hz.
#include "k4.h"

// ------------------------------------------------------------------ IDT
struct [[gnu::packed]] Gate { uint16_t off_lo, sel; uint8_t zero, type; uint16_t off_hi; };
struct [[gnu::packed]] IdtPtr { uint16_t limit; uint32_t base; };
static Gate g_idt[256];
extern "C" void irq0_stub();
extern "C" void fault_stub();
static void set_gate(int v, void (*f)())
{
    const uint32_t a = reinterpret_cast<uint32_t>(f);
    g_idt[v] = {static_cast<uint16_t>(a), 0x08, 0, 0x8E, static_cast<uint16_t>(a >> 16)};   // 32-bit interrupt gate, ring 0
}

// ------------------------------------------------------------------ timer
constexpr uint32_t kPitHz = 1193182, kTickHz = 100;
static volatile uint32_t g_ticks;
static volatile uint64_t g_last_tsc, g_min_gap = ~0ull, g_max_gap;
extern "C" void irq0_handler()
{
    const uint64_t now = k4::rdtsc();
    if (g_ticks > 0) {
        const uint64_t gap = now - g_last_tsc;
        if (gap < g_min_gap) g_min_gap = gap;
        if (gap > g_max_gap) g_max_gap = gap;
    }
    g_last_tsc = now;
    g_ticks = g_ticks + 1;
    k4::outb(0x20, 0x20);                          // end of interrupt to the master PIC
}
extern "C" [[noreturn]] void fault_handler() { k4::panic("unexpected interrupt or exception"); }

static void pic_and_pit_init()
{
    k4::outb(0x20, 0x11); k4::outb(0xA0, 0x11);    // ICW1: start init, ICW4 follows
    k4::outb(0x21, 0x20); k4::outb(0xA1, 0x28);    // ICW2: vector bases 0x20 and 0x28
    k4::outb(0x21, 0x04); k4::outb(0xA1, 0x02);    // ICW3: cascade on IRQ2
    k4::outb(0x21, 0x01); k4::outb(0xA1, 0x01);    // ICW4: 8086 mode
    k4::outb(0x21, 0xFE); k4::outb(0xA1, 0xFF);    // mask everything except IRQ0
    const uint16_t div = static_cast<uint16_t>(kPitHz / kTickHz);
    k4::outb(0x43, 0x34);                          // channel 0, low then high byte, mode 2
    k4::outb(0x40, div & 0xFF); k4::outb(0x40, div >> 8);
}

// ------------------------------------------------------------------ idle governor
static uint64_t g_tsc_hz;                          // measured below against the PIT
static uint64_t ticks_to_us(uint64_t tsc_ticks)
{
#ifdef F422_UNIT_BUG
    return k4::udiv64(tsc_ticks, g_tsc_hz);        // seconds, not microseconds
#else
    return k4::udiv64(tsc_ticks * 1000000ull, g_tsc_hz);
#endif
}
constexpr uint64_t kHaltThresholdUs = 50;          // shorter predicted idle than this: poll
static uint32_t g_halts, g_polls;
static uint64_t g_poll_loops;

static void idle_until_next_tick()
{
    const uint32_t t = g_ticks;
    const uint64_t next = g_last_tsc + k4::udiv64(g_tsc_hz, kTickHz);   // predicted next interrupt
    const uint64_t now = k4::rdtsc();
    const uint64_t predicted_us = next > now ? ticks_to_us(next - now) : 0;
#ifdef F422_FORCE_POLL
    const bool halt = false;
    (void)predicted_us;
#else
    const bool halt = predicted_us > kHaltThresholdUs;
#endif
    if (halt) {
        ++g_halts;
        while (g_ticks == t) asm volatile("sti; hlt; cli");   // sleep until an interrupt
        asm volatile("sti");
    } else {
        ++g_polls;
        asm volatile("sti");
        while (g_ticks == t) ++g_poll_loops;                 // spin, reading the counter
    }
}

extern "C" void kmain(uint32_t, uint32_t)
{
    k4::console_init();
    for (int v = 0; v < 256; ++v) set_gate(v, fault_stub);
    set_gate(0x20, irq0_stub);
    static IdtPtr p = {sizeof g_idt - 1, reinterpret_cast<uint32_t>(g_idt)};
    asm volatile("lidt %0" : : "m"(p));
    pic_and_pit_init();
    asm volatile("sti");

    // Calibrate: TSC ticks across 10 timer interrupts (100 ms if the PIT runs as programmed).
    while (g_ticks < 2) asm volatile("hlt");
    const uint32_t t0 = g_ticks; const uint64_t c0 = k4::rdtsc();
    while (g_ticks < t0 + 10) asm volatile("hlt");
    const uint64_t c1 = k4::rdtsc();
    g_tsc_hz = (c1 - c0) * (kTickHz / 10);
    k4::puts("calibration: TSC ticks per second (from 10 timer ticks) "); k4::dec(g_tsc_hz); k4::putc('\n');
    g_min_gap = ~0ull; g_max_gap = 0;

    const k4::Cpuid l6 = k4::cpuid(6);
    k4::puts("cpuid leaf 6 eax 0x"); k4::hex(l6.a, 8);
    k4::puts((l6.a >> 2) & 1 ? " (bit 2 ARAT set)" : " (bit 2 ARAT clear)"); k4::putc('\n');

    const uint32_t start = g_ticks;
    while (g_ticks < start + 100) idle_until_next_tick();   // one second of idling
    k4::puts("idle for 100 timer ticks: halts "); k4::dec(g_halts);
    k4::puts(", polls "); k4::dec(g_polls); k4::puts(", poll loop iterations "); k4::dec(g_poll_loops); k4::putc('\n');
    k4::puts("timer interval in TSC ticks: min "); k4::dec(g_min_gap); k4::puts(" max "); k4::dec(g_max_gap); k4::putc('\n');
    k4::exit_qemu(0x10);
}
