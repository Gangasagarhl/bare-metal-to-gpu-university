// intr.cc - DR302 F4-08: IDT, 8259 masking, local APIC, IOAPIC and the 1 kHz tick.
// Register offsets and bit positions of the local APIC and the IOAPIC were written from
// the author's memory of the Intel SDM Vol. 3 ("Advanced Programmable Interrupt
// Controller") and the Intel 82093AA I/O APIC datasheet; the MADT layout from the ACPI
// Specification ("Multiple APIC Description Table"). Not opened in this build: they are
// confirmed only by QEMU's models behaving as expected (see the chapter's unverified box).
#include "intr.h"
#include "../F4-01/kbase.h"
#include "../F4-02/acpi.h"

extern "C" char isr256_stubs[];

namespace {
struct Gate {
    uint16_t offset_lo, selector;
    uint8_t zero, type_attr;                 // 0x8E: present, ring 0, 32-bit interrupt gate
    uint16_t offset_hi;
} __attribute__((packed));

Gate g_idt[256];
intr::Handler g_handlers[256];
volatile uint32_t g_counts[256];
volatile uint64_t g_ms;
uint8_t g_next_vector = 0x40;

uintptr_t g_lapic = 0xFEE00000;              // replaced by the MADT value
uintptr_t g_ioapic = 0;
uint32_t g_ioapic_gsi_base = 0;
uint32_t g_ioapic_entries = 0;

struct Override { uint8_t irq; uint32_t gsi; uint16_t flags; };
Override g_ovr[16];
int g_novr = 0;

// Local APIC registers (offsets from its base).
constexpr uint32_t LAPIC_ID = 0x20, LAPIC_VER = 0x30, LAPIC_TPR = 0x80, LAPIC_EOI = 0xB0,
                   LAPIC_SVR = 0xF0;
constexpr uint8_t SPURIOUS_VECTOR = 0xFF;
constexpr uint8_t TICK_VECTOR = 0x20;
constexpr uint8_t PIC_BASE = 0xE0;           // 8259 vectors parked at 0xE0-0xEF, all masked

uint32_t lapic_read(uint32_t off) { return mmio_read<uint32_t>(g_lapic + off); }
void lapic_write(uint32_t off, uint32_t v) { mmio_write<uint32_t>(g_lapic + off, v); }

uint32_t ioapic_read(uint8_t reg)
{
    mmio_write<uint32_t>(g_ioapic + 0x00, reg);          // IOREGSEL
    return mmio_read<uint32_t>(g_ioapic + 0x10);         // IOWIN
}
void ioapic_write(uint8_t reg, uint32_t v)
{
    mmio_write<uint32_t>(g_ioapic + 0x00, reg);
    mmio_write<uint32_t>(g_ioapic + 0x10, v);
}

void tick(uint8_t) { g_ms = g_ms + 1; }

void pic_park_and_mask()
{
    // Remap the two 8259s away from the exception vectors, then mask every line: from
    // now on legacy lines arrive through the IOAPIC (same ICW sequence as DR301 F4-03).
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, PIC_BASE); outb(0xA1, PIC_BASE + 8);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0xFF); outb(0xA1, 0xFF);
}

void parse_madt()
{
    const AcpiHeader* m = acpi::find("APIC");
    if (!m) panic("no MADT");
    const auto* p = reinterpret_cast<const uint8_t*>(m);
    uint32_t lapic32;
    memcpy(&lapic32, p + 36, 4);
    g_lapic = lapic32;
    for (uint32_t off = 44; off + 2 <= m->length; off += p[off + 1]) {
        const uint8_t type = p[off], len = p[off + 1];
        if (len < 2) break;
        if (type == 1 && g_ioapic == 0) {                 // I/O APIC
            uint32_t addr, base;
            memcpy(&addr, p + off + 4, 4);
            memcpy(&base, p + off + 8, 4);
            g_ioapic = addr;
            g_ioapic_gsi_base = base;
        } else if (type == 2 && g_novr < 16) {            // interrupt source override
            Override& o = g_ovr[g_novr++];
            o.irq = p[off + 3];
            memcpy(&o.gsi, p + off + 4, 4);
            memcpy(&o.flags, p + off + 8, 2);
        }
    }
    if (!g_ioapic) panic("MADT has no IOAPIC");
}
}  // namespace

struct Frame {                         // the frame isr256.S builds (as DR301 IrqFrame)
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t vector, error_code;
    uint32_t eip, cs, eflags;
};

extern "C" void intr_dispatch(Frame* f)
{
    const uint32_t v = f->vector;
    if (v < 32) {
        kprintf("EXCEPTION %u error 0x%x at eip 0x%x\n", v, f->error_code, f->eip);
        panic("CPU exception");
    }
    g_counts[v] = g_counts[v] + 1;
    if (v >= PIC_BASE && v < PIC_BASE + 16) return;      // spurious 8259 line: no EOI anywhere
    if (v == SPURIOUS_VECTOR) return;                     // APIC spurious: no EOI by definition
    if (g_handlers[v]) g_handlers[v](static_cast<uint8_t>(v));
    lapic_write(LAPIC_EOI, 0);                            // end of interrupt at the local APIC
}

namespace intr {
void init()
{
    for (int v = 0; v < 256; ++v) {
        const auto addr = reinterpret_cast<uintptr_t>(isr256_stubs) + 16u * v;
        g_idt[v] = Gate{static_cast<uint16_t>(addr & 0xFFFF), 0x08, 0, 0x8E,
                        static_cast<uint16_t>(addr >> 16)};
    }
    struct { uint16_t limit; uint32_t base; } __attribute__((packed)) idtr{sizeof(g_idt) - 1,
                                                    reinterpret_cast<uintptr_t>(g_idt)};
    asm volatile("lidt %0" : : "m"(idtr));
    pic_park_and_mask();
    if (!acpi::init()) panic("no ACPI tables");
    parse_madt();
    g_ioapic_entries = ((ioapic_read(0x01) >> 16) & 0xFF) + 1;
    for (uint32_t i = 0; i < g_ioapic_entries; ++i) mask_gsi(g_ioapic_gsi_base + i, true);
    lapic_write(LAPIC_TPR, 0);                            // accept every priority class
    lapic_write(LAPIC_SVR, 0x100u | SPURIOUS_VECTOR);     // bit 8: APIC software enable
    // 1 kHz tick: PIT channel 0, mode 2, divisor 1193 (as in DR301 F4-03), on ISA IRQ 0.
    bool level = false, low = false;
    const uint32_t gsi = isa_to_gsi(0, &level, &low);
    set_handler(TICK_VECTOR, tick);
    outb(0x43, 0x34);
    outb(0x40, 1193 & 0xFF);
    outb(0x40, 1193 >> 8);
    route_gsi(gsi, TICK_VECTOR, level, low);
    enable();
}

uint8_t alloc_vector()
{
    if (g_next_vector >= PIC_BASE) panic("out of interrupt vectors");
    return g_next_vector++;
}
void set_handler(uint8_t vector, Handler h) { g_handlers[vector] = h; }
uint32_t count(uint8_t vector) { return g_counts[vector]; }
uint32_t lapic_id() { return lapic_read(LAPIC_ID) >> 24; }
uintptr_t lapic_base() { return g_lapic; }

uint32_t isa_to_gsi(uint8_t irq, bool* level, bool* active_low)
{
    *level = false;                                       // ISA default: edge, active high
    *active_low = false;
    for (int i = 0; i < g_novr; ++i) {
        if (g_ovr[i].irq != irq) continue;
        const uint16_t pol = g_ovr[i].flags & 3, trig = (g_ovr[i].flags >> 2) & 3;
        if (pol == 3) *active_low = true;                 // 01 high, 11 low, 00 bus default
        if (trig == 3) *level = true;                     // 01 edge, 11 level, 00 bus default
        return g_ovr[i].gsi;
    }
    return irq;                                           // identity mapped
}

void route_gsi(uint32_t gsi, uint8_t vector, bool level, bool active_low)
{
    const uint32_t pin = gsi - g_ioapic_gsi_base;
    if (pin >= g_ioapic_entries) panic("GSI not on this IOAPIC");
    const uint32_t lo = vector | (active_low ? 1u << 13 : 0) | (level ? 1u << 15 : 0);
    ioapic_write(static_cast<uint8_t>(0x11 + 2 * pin), lapic_id() << 24);   // destination
    ioapic_write(static_cast<uint8_t>(0x10 + 2 * pin), lo);                  // unmasked
}

void mask_gsi(uint32_t gsi, bool masked)
{
    const uint8_t reg = static_cast<uint8_t>(0x10 + 2 * (gsi - g_ioapic_gsi_base));
    const uint32_t lo = ioapic_read(reg);
    ioapic_write(reg, masked ? lo | (1u << 16) : lo & ~(1u << 16));
}

void print_routing()
{
    kprintf("madt: local APIC at 0x%x (this CPU's APIC ID %u, version 0x%x)\n", g_lapic, lapic_id(),
            lapic_read(LAPIC_VER) & 0xFF);
    kprintf("madt: IOAPIC at 0x%x, GSI base %u, %u redirection entries\n", g_ioapic,
            g_ioapic_gsi_base, g_ioapic_entries);
    for (int i = 0; i < g_novr; ++i) {
        const uint16_t f = g_ovr[i].flags;
        kprintf("madt: override ISA IRQ %u -> GSI %u, polarity %s, trigger %s\n", g_ovr[i].irq,
                g_ovr[i].gsi, (f & 3) == 3 ? "low" : (f & 3) == 1 ? "high" : "bus default",
                ((f >> 2) & 3) == 3 ? "level" : ((f >> 2) & 3) == 1 ? "edge" : "bus default");
    }
    for (uint32_t i = 0; i < g_ioapic_entries; ++i) {
        const uint32_t lo = ioapic_read(static_cast<uint8_t>(0x10 + 2 * i));
        if (lo & (1u << 16)) continue;                    // masked: not in use
        kprintf("ioapic: pin %u -> vector 0x%02x, %s, active %s, destination APIC %u\n", i,
                lo & 0xFF, (lo & (1u << 15)) ? "level" : "edge", (lo & (1u << 13)) ? "low" : "high",
                ioapic_read(static_cast<uint8_t>(0x11 + 2 * i)) >> 24);
    }
}
}  // namespace intr

namespace clock {
uint64_t ms()
{
    uint64_t a, b;
    do { a = g_ms; b = g_ms; } while (a != b);
    return a;
}
void sleep_ms(uint32_t n)
{
    const uint64_t end = ms() + n;
    while (ms() < end) intr::wait();
}
}  // namespace clock
