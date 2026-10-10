// madt.h - F3-24: the MADT ("APIC" table) parsed into plain structs. Pure logic: the same
// parser runs in the kernel and in the host test (madt_host.cpp) on a hand-built table.
// Entry types and offsets are from memory of the ACPI Specification's MADT section; check them.
#pragma once
#include <cstdint>

namespace madt {

struct Cpu { uint32_t acpi_uid, apic_id; bool enabled; };
struct IoApic { uint8_t id; uint32_t address, gsi_base; };
struct Override { uint8_t bus, source; uint32_t gsi; uint16_t flags; };

struct Info {
    uint64_t lapic_address = 0;
    bool has_8259 = false;            // flags bit 0 (PCAT_COMPAT): legacy PICs are present
    Cpu cpus[16];
    int ncpu = 0;
    IoApic ioapics[4];
    int nioapic = 0;
    Override overrides[16];
    int noverride = 0;
    int nmi_entries = 0, other_entries = 0;
};

// 'table' points at the whole MADT including its 36-byte header; 'length' from that header.
inline bool parse(const uint8_t* table, uint32_t length, Info& out)
{
    auto u16 = [](const uint8_t* p) { return static_cast<uint16_t>(p[0] | p[1] << 8); };
    auto u32 = [](const uint8_t* p) { return uint32_t{p[0]} | uint32_t{p[1]} << 8 | uint32_t{p[2]} << 16 | uint32_t{p[3]} << 24; };
    if (length < 44) {
        return false;
    }
    out.lapic_address = u32(table + 36);
    out.has_8259 = (u32(table + 40) & 1) != 0;
    for (uint32_t off = 44; off + 2 <= length;) {
        const uint8_t* e = table + off;
        uint8_t type = e[0], len = e[1];
        if (len < 2 || off + len > length) {
            return false;                       // a broken entry: refuse the whole table
        }
        if (type == 0 && len >= 8 && out.ncpu < 16) {            // processor local APIC
            out.cpus[out.ncpu++] = Cpu{e[2], e[3], (u32(e + 4) & 1) != 0};
        } else if (type == 9 && len >= 16 && out.ncpu < 16) {    // processor local x2APIC
            out.cpus[out.ncpu++] = Cpu{u32(e + 12), u32(e + 4), (u32(e + 8) & 1) != 0};
        } else if (type == 1 && len >= 12 && out.nioapic < 4) {  // I/O APIC
            out.ioapics[out.nioapic++] = IoApic{e[2], u32(e + 4), u32(e + 8)};
        } else if (type == 2 && len >= 10 && out.noverride < 16) {   // interrupt source override
            out.overrides[out.noverride++] = Override{e[2], e[3], u32(e + 4), u16(e + 8)};
        } else if (type == 5 && len >= 12) {                      // 64-bit local APIC address
            out.lapic_address = uint64_t{u32(e + 4)} | uint64_t{u32(e + 8)} << 32;
        } else if (type == 4 || type == 0x0A) {                   // local APIC NMI / x2APIC NMI
            ++out.nmi_entries;
        } else {
            ++out.other_entries;
        }
        off += len;
    }
    return true;
}

// How an ISA IRQ reaches the IOAPIC: identity, edge, active high unless an override says otherwise.
struct Route { uint32_t gsi; bool level, active_low; };

inline Route route_isa(const Info& m, uint8_t irq)
{
    for (int i = 0; i < m.noverride; ++i) {
        const Override& o = m.overrides[i];
        if (o.bus == 0 && o.source == irq) {
            unsigned pol = o.flags & 3, trig = (o.flags >> 2) & 3;   // 0 = conforms to the bus
            return Route{o.gsi, trig == 3, pol == 3};
        }
    }
    return Route{irq, false, false};
}

} // namespace madt
