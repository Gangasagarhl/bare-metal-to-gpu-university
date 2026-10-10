// b7_main.cc - F3-24 test kernel for milestone B7 (ACPI tables, local APIC, IOAPIC).
//   (none)          list ACPI tables, parse the MADT, switch to the APICs, take keyboard IRQ 1
//                   and serial IRQ 4 through the IOAPIC; waits for keys sent by the QEMU monitor
//   -DB7_CORRUPT    the "test patch": flips one byte of the MADT in memory, rescans: rejected
//   -DB7_NO_EOI     the forensic kernel: the keyboard handler forgets the end-of-interrupt
#include <cstdint>
#include "acpi.h"
#include "apic.h"
#include "arch.h"
#include "gdt.h"
#include "interrupts.h"
#include "kprint.h"
#include "log.h"
#include "madt.h"
#include "multiboot.h"
#include "paging.h"
#include "panic.h"
#include "pmm.h"
#include "serial.h"

namespace {

madt::Info g_madt;
volatile int g_keys = 0;
volatile int g_serial_irqs = 0;
uint8_t g_kbd_vector = 0, g_serial_vector = 0;

void on_keyboard(InterruptFrame& f)
{
    uint8_t code = arch::inb(0x60);                    // reading the data port acknowledges the controller
    g_keys = g_keys + 1;
    kprintf("irq: vector %02lx (keyboard, ISA IRQ 1) scancode %02x %s, interrupt %d\n", f.vector, unsigned{code},
            code & 0x80 ? "release" : "press  ", g_keys);
#if !defined(B7_NO_EOI)
    apic::eoi();
#endif
}

void on_serial(InterruptFrame& f)
{
    uint8_t iir = arch::inb(0x3F8 + 2);                // reading IIR clears a "transmitter empty" cause
    arch::outb(0x3F8 + 1, 0x00);                       // IER = 0: one interrupt per request
    g_serial_irqs = g_serial_irqs + 1;
    kprintf("irq: vector %02lx (COM1, ISA IRQ 4) IIR %02x (cause %u: transmitter empty), interrupt %d\n", f.vector,
            unsigned{iir}, (iir >> 1) & 7u, g_serial_irqs);
    apic::eoi();
}

void i8042_enable_irq1()
{
    while (arch::inb(0x64) & 1) {
        arch::inb(0x60);                               // drain anything the firmware left behind
    }
    arch::outb(0x64, 0x20);                            // read the controller configuration byte
    while (!(arch::inb(0x64) & 1)) {
    }
    uint8_t cfg = arch::inb(0x60);
    arch::outb(0x64, 0x60);                            // write it back with bit 0 (IRQ 1) set
    while (arch::inb(0x64) & 2) {
    }
    arch::outb(0x60, static_cast<uint8_t>(cfg | 1));
    kprintf("i8042: configuration byte %02x -> %02x\n", unsigned{cfg}, unsigned{cfg | 1u});
}

void print_madt()
{
    kprintf("madt: local APIC at %08lx, 8259 PICs %s\n", g_madt.lapic_address,
            g_madt.has_8259 ? "present (PCAT_COMPAT)" : "absent");
    for (int i = 0; i < g_madt.ncpu; ++i) {
        kprintf("madt:   CPU: ACPI uid %u, APIC id %u, %s\n", g_madt.cpus[i].acpi_uid, g_madt.cpus[i].apic_id,
                g_madt.cpus[i].enabled ? "enabled" : "disabled");
    }
    for (int i = 0; i < g_madt.nioapic; ++i) {
        kprintf("madt:   IOAPIC: id %u at %08x, first GSI %u\n", unsigned{g_madt.ioapics[i].id},
                g_madt.ioapics[i].address, g_madt.ioapics[i].gsi_base);
    }
    for (int i = 0; i < g_madt.noverride; ++i) {
        const madt::Override& o = g_madt.overrides[i];
        madt::Route r = madt::route_isa(g_madt, o.source);
        kprintf("madt:   override: ISA IRQ %2u -> GSI %2u, flags %04x (%s, active %s)\n", unsigned{o.source}, o.gsi,
                unsigned{o.flags}, r.level ? "level" : "edge", r.active_low ? "low" : "high");
    }
    kprintf("madt:   %d NMI entries, %d other entries\n", g_madt.nmi_entries, g_madt.other_entries);
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
    klog(Level::Info, "OS302 kernel, chapter F3-24 build");
    KASSERT(acpi::init());
#if defined(B7_CORRUPT)
    {
        const acpi::Header* h = acpi::find("APIC");
        KASSERT(h != nullptr);
        uint64_t phys = reinterpret_cast<uint64_t>(h) - pmm::kDirectMap;
        constexpr uint64_t kPatch = 0xffffc00000000000;          // a writable alias of the table's page
        KASSERT(paging::kernel_space().map(kPatch, phys & ~0xFFFull, paging::kPresent | paging::kWrite | paging::kNoExec));
        auto* oem_revision = reinterpret_cast<volatile uint8_t*>(kPatch + (phys & 0xFFF) + 24);
        kprintf("test patch: MADT at %08lx: OEM revision byte %02x -> %02x, checksum left alone\n", phys,
                unsigned{*oem_revision}, unsigned{*oem_revision ^ 0x40u});
        *oem_revision = static_cast<uint8_t>(*oem_revision ^ 0x40);
        KASSERT(acpi::init());
        if (acpi::find("APIC") == nullptr) {
            kprintf("acpi: no valid MADT: the kernel refuses to program the APICs from it\n");
            kprintf("B7 corrupt-table test ok: the damaged table was rejected\n");
            arch::qemu_exit(arch::kExitPass);
        }
        PANIC("the corrupted MADT was accepted");
    }
#endif
    const acpi::Header* m = acpi::find("APIC");
    KASSERT(m != nullptr);
    KASSERT(madt::parse(reinterpret_cast<const uint8_t*>(m), m->length, g_madt));
    print_madt();
    apic::disable_8259();
    KASSERT(apic::init_local(g_madt.lapic_address));
    KASSERT(apic::init_ioapic(g_madt));
    i8042_enable_irq1();
    g_kbd_vector = apic::route_isa_irq(g_madt, 1, on_keyboard);
    g_serial_vector = apic::route_isa_irq(g_madt, 4, on_serial);
    KASSERT(g_kbd_vector == 0x30 && g_serial_vector == 0x31);
    apic::print_redirection(2);                 // where the PIT's IRQ 0 went (F3-25 uses it)
    arch::sti();
    for (int i = 1; i <= 3; ++i) {              // serial: ask for a "transmitter empty" interrupt, 3 times
        arch::outb(0x3F8 + 1, 0x02);
        while (g_serial_irqs < i) {
            arch::hlt();
        }
    }
    kprintf("B7: serial IRQ 4 arrived %d times at vector %02x\n", int{g_serial_irqs}, unsigned{g_serial_vector});
    kprintf("B7: waiting for keys (send them with the QEMU monitor's sendkey)\n");
    while (g_keys < 4) {
        arch::hlt();
    }
    kprintf("B7 ok: keyboard IRQ 1 at vector %02x and serial IRQ 4 at vector %02x through the IOAPIC, "
            "%s mode, %lu spurious\n", unsigned{g_kbd_vector}, unsigned{g_serial_vector},
            apic::x2apic_mode() ? "x2APIC" : "xAPIC", apic::spurious_count());
    for (;;) {
        arch::hlt();
    }
}
