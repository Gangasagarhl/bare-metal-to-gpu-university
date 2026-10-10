// apic.cc - F3-24: 8259 shutdown, local APIC (xAPIC and x2APIC), IOAPIC, ISA IRQ routing.
// Register offsets, MSR numbers and bit positions are written from memory of the Intel SDM
// Vol. 3 APIC chapter and the 82093AA IOAPIC datasheet; the chapter's unverified box lists them.
#include "apic.h"
#include <cstdint>
#include "arch.h"
#include "kprint.h"
#include "log.h"
#include "paging.h"

namespace apic {
namespace {

constexpr uint32_t kApicBaseMsr = 0x1B;        // IA32_APIC_BASE
constexpr uint64_t kApicGlobalEnable = uint64_t{1} << 11;
constexpr uint64_t kBaseMask = ~uint64_t{0xFFF};
constexpr uint64_t kApicX2Enable = uint64_t{1} << 10;
constexpr uint32_t kRegId = 0x20, kRegVersion = 0x30, kRegTpr = 0x80, kRegEoi = 0xB0, kRegSvr = 0xF0,
                   kRegEsr = 0x280, kRegLvtTimer = 0x320, kRegLvtLint0 = 0x350, kRegLvtLint1 = 0x360,
                   kRegLvtError = 0x370;
constexpr uint32_t kMasked = 1u << 16;
constexpr uint8_t kSpuriousVector = 0xFF, kErrorVector = 0xFE;
constexpr uint64_t kMmioBase = 0xffffe80000000000;   // device registers get their own window

uint64_t g_mmio_next = kMmioBase;
volatile uint32_t* g_lapic = nullptr;         // xAPIC: the register page
bool g_x2 = false;
volatile uint32_t* g_ioapic = nullptr;        // IOREGSEL at +0x00, IOWIN at +0x10
uint32_t g_ioapic_gsi_base = 0, g_ioapic_pins = 0;
uint8_t g_next_vector = 0x30;                 // 0x20..0x2F stay free for the remapped 8259
volatile uint64_t g_spurious = 0;

uint32_t io_read(uint32_t reg)
{
    g_ioapic[0] = reg;
    return g_ioapic[4];
}

void io_write(uint32_t reg, uint32_t value)
{
    g_ioapic[0] = reg;
    g_ioapic[4] = value;
}

void on_spurious(InterruptFrame&) { g_spurious = g_spurious + 1; }   // no EOI for a spurious interrupt

void on_error(InterruptFrame&)
{
    write_lapic(kRegEsr, 0);                  // writing first latches the current errors
    klog(Level::Error, "apic: error interrupt, ESR %08x", read_lapic(kRegEsr));
    eoi();
}

} // namespace

uint64_t map_mmio(uint64_t phys, uint64_t size)
{
    uint64_t first = phys & ~0xFFFull, end = (phys + size + 0xFFF) & ~0xFFFull;
    uint64_t virt = g_mmio_next;
    for (uint64_t p = first; p < end; p += 4096, g_mmio_next += 4096) {
        uint64_t flags = paging::kPresent | paging::kWrite | paging::kCacheDisable | paging::kWriteThrough |
                         paging::kNoExec | paging::kGlobal;
        if (!paging::kernel_space().map(g_mmio_next, p, flags)) {
            return 0;
        }
    }
    return virt + (phys & 0xFFF);
}

void disable_8259()
{
    // ICW1..ICW4: start initialisation, vector bases 0x20 and 0x28, wiring, 8086 mode
    constexpr uint16_t kPic1 = 0x20, kPic2 = 0xA0;
    arch::outb(kPic1, 0x11);
    arch::outb(kPic2, 0x11);
    arch::outb(kPic1 + 1, 0x20);
    arch::outb(kPic2 + 1, 0x28);
    arch::outb(kPic1 + 1, 0x04);   // a slave on line 2
    arch::outb(kPic2 + 1, 0x02);   // slave identity 2
    arch::outb(kPic1 + 1, 0x01);
    arch::outb(kPic2 + 1, 0x01);
    arch::outb(kPic1 + 1, 0xFF);   // mask everything: from now on the IOAPIC delivers
    arch::outb(kPic2 + 1, 0xFF);
    kprintf("apic: 8259 PICs remapped to 0x20-0x2f and fully masked (masks %02x %02x)\n",
            unsigned{arch::inb(kPic1 + 1)}, unsigned{arch::inb(kPic2 + 1)});
}

uint32_t read_lapic(uint32_t offset)
{
    if (g_x2) {
        return static_cast<uint32_t>(arch::rdmsr(0x800 + (offset >> 4)));
    }
    return g_lapic[offset / 4];
}

void write_lapic(uint32_t offset, uint32_t value)
{
    if (g_x2) {
        arch::wrmsr(0x800 + (offset >> 4), value);
    } else {
        g_lapic[offset / 4] = value;
    }
}

bool x2apic_mode() { return g_x2; }

uint32_t local_id()
{
    uint32_t id = read_lapic(kRegId);
    return g_x2 ? id : id >> 24;              // xAPIC keeps the 8-bit ID in bits 31:24
}

void eoi() { write_lapic(kRegEoi, 0); }

uint64_t spurious_count() { return g_spurious; }

bool init_local(uint64_t lapic_phys)
{
    uint32_t a, b, c, d;
    arch::cpuid(1, 0, a, b, c, d);
    bool has_apic = (d >> 9) & 1, has_x2 = (c >> 21) & 1;
    if (!has_apic) {
        klog(Level::Error, "apic: CPUID says there is no local APIC");
        return false;
    }
    uint64_t base = arch::rdmsr(kApicBaseMsr);
    kprintf("apic: IA32_APIC_BASE %016lx (base %08lx, %s, %s), CPUID x2APIC %s\n", base, base & kBaseMask,
            (base & (uint64_t{1} << 8)) ? "boot CPU" : "application CPU", (base & kApicGlobalEnable) ? "enabled" : "disabled",
            has_x2 ? "yes" : "no");
    if ((base & kBaseMask) != lapic_phys) {
        klog(Level::Warn, "apic: MADT says %08lx, MSR says %08lx: using the MSR", lapic_phys, base & kBaseMask);
    }
    if (has_x2) {
        arch::wrmsr(kApicBaseMsr, base | kApicGlobalEnable | kApicX2Enable);
        g_x2 = true;
    } else {
        arch::wrmsr(kApicBaseMsr, base | kApicGlobalEnable);
        g_lapic = reinterpret_cast<volatile uint32_t*>(map_mmio(base & kBaseMask, 4096));
        if (g_lapic == nullptr) {
            return false;
        }
    }
    idt::set_handler(kSpuriousVector, on_spurious);
    idt::set_handler(kErrorVector, on_error);
    write_lapic(kRegTpr, 0);                          // accept every priority
    write_lapic(kRegLvtTimer, kMasked);               // the timer is F3-25's business
    write_lapic(kRegLvtLint0, kMasked);               // the 8259's ExtINT input: off
    write_lapic(kRegLvtLint1, 4u << 8);               // LINT1 = NMI, as on PC hardware
    write_lapic(kRegLvtError, kErrorVector);
    write_lapic(kRegSvr, (1u << 8) | kSpuriousVector);   // software enable + spurious vector
    uint32_t ver = read_lapic(kRegVersion);
    kprintf("apic: local APIC in %s mode, id %u, version %02x, %u LVT entries, SVR %08x\n",
            g_x2 ? "x2APIC (MSR)" : "xAPIC (MMIO)", local_id(), ver & 0xFF, ((ver >> 16) & 0xFF) + 1,
            read_lapic(kRegSvr));
    return true;
}

bool init_ioapic(const madt::Info& m)
{
    if (m.nioapic == 0) {
        klog(Level::Error, "apic: the MADT lists no IOAPIC");
        return false;
    }
    g_ioapic = reinterpret_cast<volatile uint32_t*>(map_mmio(m.ioapics[0].address, 0x20));
    if (g_ioapic == nullptr) {
        return false;
    }
    g_ioapic_gsi_base = m.ioapics[0].gsi_base;
    uint32_t ver = io_read(1);
    g_ioapic_pins = ((ver >> 16) & 0xFF) + 1;
    for (uint32_t pin = 0; pin < g_ioapic_pins; ++pin) {
        io_write(0x10 + 2 * pin, kMasked);
        io_write(0x11 + 2 * pin, 0);
    }
    kprintf("apic: IOAPIC id %u at %08x, version %02x, %u pins (GSI %u-%u), all masked\n",
            (io_read(0) >> 24) & 0xF, m.ioapics[0].address, ver & 0xFF, g_ioapic_pins, g_ioapic_gsi_base,
            g_ioapic_gsi_base + g_ioapic_pins - 1);
    return true;
}

void print_redirection(uint32_t gsi)
{
    uint32_t pin = gsi - g_ioapic_gsi_base;
    uint32_t lo = io_read(0x10 + 2 * pin), hi = io_read(0x11 + 2 * pin);
    kprintf("apic:   GSI %2u: redirection %08x%08x = vector %02x, %s, active %s, %s, destination %u\n", gsi, hi, lo,
            lo & 0xFF, (lo >> 15) & 1 ? "level" : "edge", (lo >> 13) & 1 ? "low" : "high",
            (lo >> 16) & 1 ? "masked" : "unmasked", hi >> 24);
}

uint8_t route_isa_irq(const madt::Info& m, uint8_t irq, IrqHandler h)
{
    madt::Route r = madt::route_isa(m, irq);
    if (r.gsi < g_ioapic_gsi_base || r.gsi >= g_ioapic_gsi_base + g_ioapic_pins || g_next_vector > 0xEF) {
        return 0;
    }
    uint8_t vector = g_next_vector++;
    idt::set_handler(vector, h);
    uint32_t pin = r.gsi - g_ioapic_gsi_base;
    uint32_t lo = vector | (r.level ? 1u << 15 : 0) | (r.active_low ? 1u << 13 : 0);   // fixed, physical
    io_write(0x11 + 2 * pin, local_id() << 24);
    io_write(0x10 + 2 * pin, lo);             // written last: bit 16 clear = unmasked
    kprintf("apic: ISA IRQ %u -> GSI %u (%s) -> vector %02x\n", irq, r.gsi,
            r.gsi != irq ? "moved by an override" : "identity", vector);
    print_redirection(r.gsi);
    return vector;
}

} // namespace apic
