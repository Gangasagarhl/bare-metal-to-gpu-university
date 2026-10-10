// lapic.cc - legacy PIC masking, local APIC, APIC timer and PIT-based delays.
// Register offsets and bit positions follow the Intel SDM Vol. 3 chapter "Advanced
// Programmable Interrupt Controller (APIC)" and the Intel 8254 datasheet (titles only;
// see the chapter's unverified box). OS302 (F3-24, F3-25) covers them properly.
#include "hooks.h"

namespace k {

namespace {
volatile uint32_t* lapic_base()
{
    return reinterpret_cast<volatile uint32_t*>(rdmsr(0x1B) & 0xFFFFF000);   // IA32_APIC_BASE
}
uint32_t rd(uint32_t off) { return lapic_base()[off / 4]; }
void wr(uint32_t off, uint32_t v) { lapic_base()[off / 4] = v; }

constexpr uint32_t kId = 0x20, kTpr = 0x80, kEoi = 0xB0, kSvr = 0xF0, kIcrLo = 0x300,
                   kIcrHi = 0x310, kLvtTimer = 0x320, kInit = 0x380, kCur = 0x390, kDiv = 0x3E0;
uint32_t ticks_per_10ms = 0;
uint64_t tsc_ms = 0;
}  // namespace

void pic_disable()
{
    // Move the 8259 PICs to vectors 0xE0-0xEF (away from CPU exceptions), then mask all.
    const uint8_t seq[][2] = {{0x20, 0x11}, {0xA0, 0x11}, {0x21, 0xE0}, {0xA1, 0xE8},
                              {0x21, 0x04}, {0xA1, 0x02}, {0x21, 0x01}, {0xA1, 0x01},
                              {0x21, 0xFF}, {0xA1, 0xFF}};
    for (auto& s : seq) {
        outb(s[0], s[1]);
    }
}

void lapic_init_cpu()
{
    wr(kSvr, 0x100 | kVecSpurious);   // software enable + spurious vector
    wr(kTpr, 0);                       // accept all priorities
}

uint32_t lapic_id() { return rd(kId) >> 24; }
void lapic_eoi() { wr(kEoi, 0); }

void lapic_send_ipi(uint32_t apic_id, uint8_t vector)
{
    uint64_t f = irq_save();
    while (rd(kIcrLo) & (1u << 12)) {   // delivery status: previous IPI still pending
        cpu_relax();
    }
    wr(kIcrHi, apic_id << 24);
    wr(kIcrLo, vector);                 // fixed delivery, physical destination
    irq_restore(f);
}

void pit_wait_us(uint32_t us)   // PIT channel 2, mode 0, polled (input clock 1193182 Hz)
{
    uint32_t count = uint32_t(1193182ull * us / 1000000);
    if (count > 0xFFFF) {
        count = 0xFFFF;
    }
    uint8_t ctl = inb(0x61) & 0xFC;   // gate off, speaker off
    outb(0x61, ctl);
    outb(0x43, 0xB0);                 // channel 2, low then high byte, mode 0
    outb(0x42, uint8_t(count));
    outb(0x42, uint8_t(count >> 8));
    outb(0x61, ctl | 1);              // gate on: counting starts
    while ((inb(0x61) & 0x20) == 0) { // OUT2 goes high at terminal count
        cpu_relax();
    }
    outb(0x61, ctl);
}

void lapic_calibrate()   // on the boot CPU: APIC timer ticks and TSC cycles per 10 ms
{
    wr(kDiv, 0x3);                    // divide by 16
    wr(kLvtTimer, 1u << 16);          // masked, one-shot
    wr(kInit, 0xFFFFFFFF);
    uint64_t t0 = rdtsc();
    pit_wait_us(10000);
    uint64_t t1 = rdtsc();
    ticks_per_10ms = 0xFFFFFFFF - rd(kCur);
    wr(kInit, 0);
    tsc_ms = (t1 - t0) / 10;
    kprintf("calibration against the PIT: APIC timer %u ticks per 10 ms (divide 16), "
            "TSC %lu cycles per ms\n", ticks_per_10ms, tsc_ms);
}

uint64_t tsc_per_ms() { return tsc_ms; }

void lapic_timer_start(int hz)
{
    wr(kDiv, 0x3);
    wr(kLvtTimer, kVecTimer | (1u << 17));   // periodic mode
    wr(kInit, uint32_t(uint64_t(ticks_per_10ms) * 100 / uint64_t(hz)));
}

}  // namespace k
