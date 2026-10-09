// acpi.cc - DR301: RSDP search, RSDT/XSDT walk, MCFG lookup.
// Layouts written from the author's memory of the ACPI Specification ("ACPI Software
// Programming Model": RSDP, RSDT, XSDT) and the PCI Firmware Specification (MCFG);
// checked only by QEMU's firmware tables being found and parsed (see F4-02).
#include "acpi.h"
#include "kbase.h"

namespace {
const AcpiHeader* g_sdt = nullptr;   // RSDT or XSDT
bool g_xsdt = false;

bool sum_ok(const void* p, uint32_t n)
{
    uint8_t s = 0;
    for (uint32_t i = 0; i < n; ++i) s = static_cast<uint8_t>(s + static_cast<const uint8_t*>(p)[i]);
    return s == 0;                   // every ACPI checksum makes the bytes sum to 0 mod 256
}
}  // namespace

namespace acpi {
bool init()
{
    // The RSDP sits on a 16-byte boundary in the BIOS area 0xE0000-0xFFFFF (or in the
    // first KiB of the EBDA, not searched here); it starts with "RSD PTR ".
    for (uintptr_t p = 0xE0000; p < 0x100000; p += 16) {
        const auto* b = reinterpret_cast<const uint8_t*>(p);
        if (memcmp(b, "RSD PTR ", 8) != 0 || !sum_ok(b, 20)) continue;
        const uint8_t rev = b[15];
        uint32_t rsdt;
        memcpy(&rsdt, b + 16, 4);
        if (rev >= 2) {
            uint64_t xsdt;
            memcpy(&xsdt, b + 24, 8);
            if (xsdt != 0 && xsdt < 0x100000000ull) {
                g_sdt = reinterpret_cast<const AcpiHeader*>(static_cast<uintptr_t>(xsdt));
                g_xsdt = true;
            }
        }
        if (!g_sdt) g_sdt = reinterpret_cast<const AcpiHeader*>(static_cast<uintptr_t>(rsdt));
        kprintf("acpi: RSDP at 0x%x revision %u, using %s at 0x%x\n", static_cast<uint32_t>(p), rev,
                g_xsdt ? "XSDT" : "RSDT", reinterpret_cast<uintptr_t>(g_sdt));
        return sum_ok(g_sdt, g_sdt->length);
    }
    return false;
}

const AcpiHeader* find(const char sig[4])
{
    if (!g_sdt) return nullptr;
    const uint32_t entry = g_xsdt ? 8 : 4;
    const uint32_t n = (g_sdt->length - sizeof(AcpiHeader)) / entry;
    const auto* base = reinterpret_cast<const uint8_t*>(g_sdt) + sizeof(AcpiHeader);
    for (uint32_t i = 0; i < n; ++i) {
        uint64_t addr = 0;
        memcpy(&addr, base + i * entry, entry);
        const auto* h = reinterpret_cast<const AcpiHeader*>(static_cast<uintptr_t>(addr));
        if (memcmp(h->signature, sig, 4) == 0 && sum_ok(h, h->length)) return h;
    }
    return nullptr;
}

bool ecam(uint64_t& base, uint8_t& bus_start, uint8_t& bus_end)
{
    const AcpiHeader* m = find("MCFG");
    if (!m) return false;
    // MCFG: header, 8 reserved bytes, then 16-byte entries:
    // base address (8), PCI segment group (2), start bus (1), end bus (1), reserved (4).
    const auto* e = reinterpret_cast<const uint8_t*>(m) + 44;
    if (m->length < 44 + 16) return false;
    memcpy(&base, e, 8);
    uint16_t segment;
    memcpy(&segment, e + 8, 2);
    bus_start = e[10];
    bus_end = e[11];
    return segment == 0;
}
}  // namespace acpi
