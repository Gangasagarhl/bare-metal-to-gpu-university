// pci.cc - DR301 F4-02: configuration access (legacy ports and ECAM), bridge recursion,
// BAR sizing. Paging is off in the lab kernel, so the ECAM window is used directly.
#include "pci.h"
#include "kbase.h"

namespace {
uint64_t g_ecam = 0;
uint8_t g_bus_start = 0, g_bus_end = 0;

// Legacy configuration mechanism: write the address to port 0xCF8, data at 0xCFC.
// Bit 31 enables the cycle; bus 23:16, device 15:11, function 10:8, register 7:2.
uint32_t legacy_addr(PciAddr a, uint16_t off)
{
    return 0x80000000u | (uint32_t{a.bus} << 16) | (uint32_t{a.dev} << 11) | (uint32_t{a.fn} << 8) |
           (off & 0xFCu);
}

// ECAM: each function owns 4 KiB at base + (bus << 20 | device << 15 | function << 12).
uintptr_t ecam_addr(PciAddr a, uint16_t off)
{
    return static_cast<uintptr_t>(g_ecam + ((uint64_t{a.bus} - g_bus_start) << 20) +
                                  (uint64_t{a.dev} << 15) + (uint64_t{a.fn} << 12) + off);
}

void scan_bus(uint8_t bus, void (*fn)(PciAddr));

void scan_function(PciAddr a, void (*fn)(PciAddr))
{
    fn(a);
    const uint8_t hdr = pci::read8(a, pcireg::HEADER_TYPE) & pcireg::HDR_MASK;
    if (hdr == pcireg::HDR_BRIDGE) {
        const uint8_t secondary = pci::read8(a, pcireg::SECONDARY_BUS);
        if (secondary != 0 && secondary > a.bus) scan_bus(secondary, fn);  // firmware numbered it
    }
}

void scan_bus(uint8_t bus, void (*fn)(PciAddr))
{
    for (uint8_t dev = 0; dev < 32; ++dev) {
        PciAddr a{bus, dev, 0};
        if (pci::read16(a, pcireg::VENDOR_ID) == 0xFFFF) continue;   // nobody answered
        scan_function(a, fn);
        if (pci::read8(a, pcireg::HEADER_TYPE) & pcireg::HDR_MULTIFUNCTION) {
            for (uint8_t f = 1; f < 8; ++f) {
                PciAddr b{bus, dev, f};
                if (pci::read16(b, pcireg::VENDOR_ID) != 0xFFFF) scan_function(b, fn);
            }
        }
    }
}
}  // namespace

namespace pci {
bool trace_bars = false;

void use_ecam(uint64_t base, uint8_t bus_start, uint8_t bus_end)
{
    g_ecam = base;
    g_bus_start = bus_start;
    g_bus_end = bus_end;
}
bool ecam_active() { return g_ecam != 0; }

uint32_t read32(PciAddr a, uint16_t off)
{
    if (g_ecam) return mmio_read<uint32_t>(ecam_addr(a, off & 0xFFC));
    outl(0xCF8, legacy_addr(a, off));
    return inl(0xCFC);
}
void write32(PciAddr a, uint16_t off, uint32_t v)
{
    if (g_ecam) { mmio_write<uint32_t>(ecam_addr(a, off & 0xFFC), v); return; }
    outl(0xCF8, legacy_addr(a, off));
    outl(0xCFC, v);
}
// Narrow accesses are done as 32-bit reads and shifts, which works with both mechanisms.
uint16_t read16(PciAddr a, uint16_t off) { return static_cast<uint16_t>(read32(a, off) >> ((off & 2) * 8)); }
uint8_t read8(PciAddr a, uint16_t off) { return static_cast<uint8_t>(read32(a, off) >> ((off & 3) * 8)); }
void write16(PciAddr a, uint16_t off, uint16_t v)
{
    uint32_t w = read32(a, off);
    const unsigned sh = (off & 2) * 8;
    w = (w & ~(0xFFFFu << sh)) | (uint32_t{v} << sh);
    // Careful: for COMMAND (0x04) this also writes STATUS (0x06), whose error bits are
    // write-1-to-clear. Writing back what we read clears any error bits that were set.
    write32(a, off, w);
}

void enable(PciAddr a, uint16_t bits) { write16(a, pcireg::COMMAND, read16(a, pcireg::COMMAND) | bits); }

int size_bars(PciAddr a, Bar out[6], int nbars)
{
    // Sizing writes all ones to a BAR, so the device must not decode while we do it.
    const uint16_t cmd = read16(a, pcireg::COMMAND);
    write16(a, pcireg::COMMAND, cmd & ~(pcireg::CMD_IO | pcireg::CMD_MEMORY));
    int used = 0;
    for (int i = 0; i < 6; ++i) out[i] = Bar{};
    for (int i = 0; i < nbars; ++i) {
        const uint16_t off = static_cast<uint16_t>(pcireg::BAR0 + 4 * i);
        const uint32_t orig = read32(a, off);
        write32(a, off, 0xFFFFFFFFu);
        const uint32_t probe = read32(a, off);
        write32(a, off, orig);
        if (probe == 0) continue;                       // BAR not implemented
        if (trace_bars) kprintf("  raw BAR%d original 0x%08x after-ones 0x%08x\n", i, orig, probe);
        Bar& b = out[i];
        b.io = orig & pcireg::BAR_IO;
        if (b.io) {
            b.addr = orig & ~0x3u;
            b.size = (~(probe & ~0x3u) + 1) & 0xFFFF;   // I/O space is 16 bits on x86
        } else {
            b.prefetch = orig & pcireg::BAR_PREFETCH;
#ifndef F402_BAR64_BUG
            b.is64 = (orig & pcireg::BAR_TYPE_MASK) == pcireg::BAR_TYPE_64;
#endif
            uint64_t addr = orig & ~0xFu, mask = probe & ~0xFu;
            if (b.is64) {                               // the next BAR holds the upper half
                const uint16_t hi = static_cast<uint16_t>(off + 4);
                const uint32_t orig_hi = read32(a, hi);
                write32(a, hi, 0xFFFFFFFFu);
                const uint32_t probe_hi = read32(a, hi);
                write32(a, hi, orig_hi);
                if (trace_bars) kprintf("  raw BAR%d original 0x%08x after-ones 0x%08x (upper half)\n", i + 1, orig_hi, probe_hi);
                addr |= uint64_t{orig_hi} << 32;
                mask |= uint64_t{probe_hi} << 32;
            } else {
                mask |= 0xFFFFFFFF00000000ull;          // treat as a 64-bit mask for the maths
            }
            b.addr = addr;
            b.size = ~mask + 1;
            if (b.is64) ++i;                            // the upper half is not a BAR of its own
        }
        ++used;
    }
    write16(a, pcireg::COMMAND, cmd);                   // decoding back as the firmware left it
    return used;
}

uint16_t find_cap(PciAddr a, uint8_t id)
{
    if (!(read16(a, pcireg::STATUS) & pcireg::STATUS_CAP_LIST)) return 0;
    uint8_t p = read8(a, pcireg::CAP_PTR) & 0xFC;
    for (int guard = 0; p && guard < 48; ++guard) {     // a broken list must not loop forever
        if (read8(a, p) == id) return p;
        p = read8(a, static_cast<uint16_t>(p + 1)) & 0xFC;
    }
    return 0;
}

void enumerate(void (*fn)(PciAddr a)) { scan_bus(0, fn); }
}  // namespace pci
