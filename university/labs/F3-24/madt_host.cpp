// madt_host.cpp - F3-24: host test of madt.h and acpi::checksum_ok on hand-built tables.
// The table imitates the one QEMU 8.2.2 (pc machine) printed in boot.out, plus an x2APIC entry
// and a broken entry, so that the parser's rejection path and the override decoding are tested.
#include <cstdint>
#include <cstdio>
#include <vector>
#include "acpi.h"
#include "madt.h"

int g_fail = 0;
#define CHECK(c) do { bool ok_ = (c); std::printf("%-4s %s\n", ok_ ? "ok" : "FAIL", #c); g_fail += ok_ ? 0 : 1; } while (0)

struct Builder {
    std::vector<uint8_t> b;
    void u8(unsigned v) { b.push_back(static_cast<uint8_t>(v)); }
    void u16(unsigned v) { u8(v & 0xFF); u8(v >> 8); }
    void u32(uint32_t v) { u16(v & 0xFFFF); u16(v >> 16); }
    void finish()                                  // length and checksum into the header
    {
        uint32_t len = static_cast<uint32_t>(b.size());
        for (int i = 0; i < 4; ++i) b[4 + i] = static_cast<uint8_t>(len >> (8 * i));
        b[9] = 0;
        uint8_t sum = 0;
        for (uint8_t x : b) sum = static_cast<uint8_t>(sum + x);
        b[9] = static_cast<uint8_t>(0x100 - sum);
    }
};

Builder qemu_like_madt(bool with_x2, bool broken)
{
    Builder t;
    for (char c : {'A', 'P', 'I', 'C'}) t.u8(static_cast<uint8_t>(c));
    t.u32(0); t.u8(3); t.u8(0);                    // length, revision 3, checksum (filled later)
    for (char c : {'B', 'O', 'C', 'H', 'S', ' '}) t.u8(static_cast<uint8_t>(c));
    for (int i = 0; i < 8; ++i) t.u8('X');
    t.u32(1); t.u32(0); t.u32(0);
    t.u32(0xFEE00000); t.u32(1);                   // local APIC address, PCAT_COMPAT
    t.u8(0); t.u8(8); t.u8(0); t.u8(0); t.u32(1);  // CPU uid 0, APIC id 0, enabled
    t.u8(1); t.u8(12); t.u8(0); t.u8(0); t.u32(0xFEC00000); t.u32(0);   // IOAPIC 0, GSI base 0
    t.u8(2); t.u8(10); t.u8(0); t.u8(0); t.u32(2); t.u16(0);            // IRQ 0 -> GSI 2
    for (unsigned irq : {5u, 9u, 10u, 11u}) {      // level, active high (flags 0x000d)
        t.u8(2); t.u8(10); t.u8(0); t.u8(irq); t.u32(irq); t.u16(0x000D);
    }
    t.u8(2); t.u8(10); t.u8(0); t.u8(7); t.u32(7); t.u16(0x000F);       // made up: level, active low
    t.u8(4); t.u8(6); t.u8(0xFF); t.u16(0); t.u8(1);                    // LAPIC NMI on LINT1
    if (with_x2) {
        t.u8(9); t.u8(16); t.u16(0); t.u32(300); t.u32(1); t.u32(7);     // x2APIC id 300, uid 7
    }
    if (broken) {
        t.u8(1); t.u8(40);                         // claims 40 bytes, the table ends sooner
    }
    t.finish();
    return t;
}

int main()
{
    Builder t = qemu_like_madt(true, false);
    CHECK(acpi::checksum_ok(t.b.data(), static_cast<uint32_t>(t.b.size())));
    t.b[24] ^= 0x40;                               // the same one-byte patch as the B7_CORRUPT kernel
    CHECK(!acpi::checksum_ok(t.b.data(), static_cast<uint32_t>(t.b.size())));
    t.b[24] ^= 0x40;

    madt::Info m;
    CHECK(madt::parse(t.b.data(), static_cast<uint32_t>(t.b.size()), m));
    CHECK(m.lapic_address == 0xFEE00000 && m.has_8259);
    CHECK(m.ncpu == 2 && m.cpus[0].apic_id == 0 && m.cpus[1].apic_id == 300 && m.cpus[1].acpi_uid == 7);
    CHECK(m.nioapic == 1 && m.ioapics[0].address == 0xFEC00000 && m.ioapics[0].gsi_base == 0);
    CHECK(m.noverride == 6 && m.nmi_entries == 1);
    madt::Route r0 = madt::route_isa(m, 0), r1 = madt::route_isa(m, 1), r9 = madt::route_isa(m, 9),
                r7 = madt::route_isa(m, 7);
    CHECK(r0.gsi == 2 && !r0.level && !r0.active_low);   // the PIT moves to GSI 2
    CHECK(r1.gsi == 1 && !r1.level && !r1.active_low);   // no override: identity, edge, high
    CHECK(r9.gsi == 9 && r9.level && !r9.active_low);    // ACPI SCI: level, high
    CHECK(r7.gsi == 7 && r7.level && r7.active_low);

    Builder bad = qemu_like_madt(false, true);
    madt::Info m2;
    CHECK(acpi::checksum_ok(bad.b.data(), static_cast<uint32_t>(bad.b.size())));   // a valid sum...
    CHECK(!madt::parse(bad.b.data(), static_cast<uint32_t>(bad.b.size()), m2));     // ...but a broken entry
    std::printf("%s: %d failure(s)\n", g_fail == 0 ? "PASS" : "FAIL", g_fail);
    return g_fail == 0 ? 0 : 1;
}
