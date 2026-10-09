// f414_main.cc - F4-14 lab kernel: a device report for the machine it boots on.
// Prints the CPU identification, every PCI function with its IDs and class code, the ACPI
// tables with their checksums, and the DSDT bytes (for the host-side _HID scanner), then
// ends the QEMU run through isa-debug-exit (exit status 33 = pass).
#include "k4.h"
#include "pci4.h"
#include "acpi4.h"

static void cpu_line()
{
    const k4::Cpuid v = k4::cpuid(0);
    char vendor[13];
    const uint32_t regs[3] = {v.b, v.d, v.c};          // the vendor string is in EBX, EDX, ECX
    for (int r = 0; r < 3; ++r)
        for (int i = 0; i < 4; ++i) vendor[4 * r + i] = static_cast<char>(regs[r] >> (8 * i));
    vendor[12] = 0;
    const k4::Cpuid s = k4::cpuid(1);
    uint32_t family = (s.a >> 8) & 0xF, model = (s.a >> 4) & 0xF;
    const uint32_t stepping = s.a & 0xF;
    if (family == 0xF) family += (s.a >> 20) & 0xFF;
    if (family == 0x6 || family >= 0xF) model += ((s.a >> 16) & 0xF) << 4;
    k4::puts("cpu: vendor \""); k4::puts(vendor);
    k4::puts("\" family 0x"); k4::hex(family, 2);
    k4::puts(" model 0x"); k4::hex(model, 2);
    k4::puts(" stepping "); k4::dec(stepping);
    k4::puts(" hypervisor-bit "); k4::dec((s.c >> 31) & 1);
    k4::putc('\n');
}

static void print_function(const pci4::Function& f, void*)
{
    k4::puts("pci ");
    k4::hex(f.at.bus, 2); k4::putc(':'); k4::hex(f.at.dev, 2); k4::putc('.'); k4::hex(f.at.fn, 1);
    k4::puts(" id "); k4::hex(f.vendor, 4); k4::putc(':'); k4::hex(f.device, 4);
    k4::puts(" sub "); k4::hex(f.subvendor, 4); k4::putc(':'); k4::hex(f.subdevice, 4);
    k4::puts(" rev "); k4::hex(f.revision, 2);
    k4::puts(" class "); k4::hex(f.base_class, 2); k4::putc(' ');
    k4::hex(f.sub_class, 2); k4::putc(' '); k4::hex(f.prog_if, 2);
    k4::puts(" hdr "); k4::hex(f.header_type, 2);
    k4::putc('\n');
    if ((f.header_type & 0x7F) != 0) return;            // BARs below are for type-0 headers only
    for (int i = 0; i < 6; ++i) {
        const pci4::Bar b = pci4::read_bar(f.at, i);
        if (b.size == 0) continue;
        k4::puts("    bar"); k4::dec(static_cast<uint64_t>(i));
        k4::puts(b.io ? " io    " : (b.is64 ? " mem64 " : " mem32 "));
        k4::puts("base 0x"); k4::hex(b.base, 8); k4::puts(" size 0x"); k4::hex(b.size, 8);
        k4::putc('\n');
        if (b.is64) ++i;                                 // a 64-bit BAR uses two slots
    }
}

extern "C" void kmain(uint32_t magic, uint32_t)
{
    k4::console_init();
    k4::line("F4-14 device report");
    if (magic != 0x2BADB002) k4::panic("not started by a Multiboot loader");
    cpu_line();
    const int n = pci4::enumerate(print_function, nullptr);
    k4::puts("pci: "); k4::dec(static_cast<uint64_t>(n)); k4::line(" functions");
    if (!acpi4::init()) k4::panic("no ACPI RSDP found");
    for (int i = 0; i < acpi4::count(); ++i) {
        const acpi4::Table t = acpi4::at(i);
        k4::puts("acpi table "); k4::puts(t.sig); k4::puts(" length "); k4::dec(t.length);
        k4::puts(t.checksum_ok ? " checksum ok" : " CHECKSUM BAD"); k4::putc('\n');
    }
    acpi4::Table d;
    if (!acpi4::dsdt(d)) k4::panic("no DSDT behind the FADT");
    k4::puts("acpi table DSDT length "); k4::dec(d.length);
    k4::line(d.checksum_ok ? " checksum ok (dump follows)" : " CHECKSUM BAD");
    acpi4::dump_hex(d, "dsdt");
    k4::line("devreport: done");
    k4::exit_qemu(0x10);
}
