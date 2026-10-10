// gpuprobe.cc - DR405 F4-44: read-only GPU probing from the lab kernel.
#include "gpuprobe.h"
#include "kbase.h"
#include "romparse.h"

namespace {
int g_cfg_writes = 0;
uint32_t g_primary_hash = 0;

const char* cap_name(uint8_t id)
{
    switch (id) {
    case pcireg::CAP_PM: return "power management";
    case pcireg::CAP_MSI: return "MSI";
    case pcireg::CAP_VENDOR: return "vendor-specific";
    case pcireg::CAP_EXP: return "PCI Express";
    case pcireg::CAP_MSIX: return "MSI-X";
    default: return "other";
    }
}

void bars(PciAddr a)
{
    for (int i = 0; i < 6; ++i) {
        const uint16_t off = static_cast<uint16_t>(pcireg::BAR0 + 4 * i);
        const uint32_t raw = pci::read32(a, off);
        if (raw == 0) continue;
        if (raw & pcireg::BAR_IO) {
            kprintf("  BAR%d raw 0x%08x: I/O ports at 0x%x\n", i, raw, raw & ~3u);
            continue;
        }
        const bool is64 = (raw & pcireg::BAR_TYPE_MASK) == pcireg::BAR_TYPE_64;
        uint64_t addr = raw & ~0xFu;
        if (is64) addr |= uint64_t{pci::read32(a, static_cast<uint16_t>(off + 4))} << 32;
        kprintf("  BAR%d raw 0x%08x: memory at 0x%lx, %s, %s (size not measured: sizing writes the BAR)\n", i, raw,
                addr, is64 ? "64-bit" : "32-bit", (raw & pcireg::BAR_PREFETCH) ? "prefetchable" : "non-prefetchable");
        if (is64) ++i;
    }
}

void caps(PciAddr a)
{
    if (!(pci::read16(a, pcireg::STATUS) & pcireg::STATUS_CAP_LIST)) {
        kprintf("  capabilities: none (status bit 4 clear)\n");
        return;
    }
    uint8_t p = pci::read8(a, pcireg::CAP_PTR) & 0xFC;
    for (int guard = 0; p && guard < 48; ++guard) {
        const uint8_t id = pci::read8(a, p);
        kprintf("  capability at 0x%02x: id 0x%02x (%s)\n", p, id, cap_name(id));
        p = pci::read8(a, static_cast<uint16_t>(p + 1)) & 0xFC;
    }
}

void report(const char* where, const rom::Info& r, uint32_t hash)
{
    kprintf("  %s: 55AA %s, PCIR at 0x%04x %s, vendor %04x device %04x class %06x, image %u bytes, code type %u,"
            " checksum %s (sum 0x%02x), fnv1a 0x%08x\n", where, r.header_ok ? "yes" : "NO", r.pcir_offset,
            r.pcir_ok ? "ok" : "BAD", r.vendor, r.device, r.class_code, r.image_length, r.code_type,
            r.checksum_ok ? "ok" : "BAD", r.sum, hash);
}

void expansion_rom(PciAddr a)
{
    const uint32_t raw = pci::read32(a, 0x30);          // PCI_ROM_ADDRESS in a type 0 header
    const uint32_t base = raw & ~0x7FFu;
    kprintf("  ROM BAR raw 0x%08x: address 0x%x, decode %s\n", raw, base, (raw & 1) ? "enabled" : "disabled");
    if (base == 0) {
        kprintf("  ROM: no address assigned by firmware; a read-only tool does not assign one\n");
        return;
    }
    if (!(pci::read16(a, pcireg::COMMAND) & pcireg::CMD_MEMORY)) {
        kprintf("  ROM: memory decoding is off in the command register; not touching it\n");
        return;
    }
#ifndef F444_FORGET_ROM_ENABLE
    pci::write32(a, 0x30, raw | 1);                     // write 1 of 2: let the ROM answer reads
    ++g_cfg_writes;
#endif
    const volatile uint8_t* img = reinterpret_cast<const volatile uint8_t*>(static_cast<uintptr_t>(base));
    kprintf("  ROM first bytes: %02x %02x %02x %02x\n", img[0], img[1], img[2], img[3]);
    const rom::Info r = rom::parse(img, 256 * 1024);
    const uint32_t h = r.checksum_ok ? rom::fnv1a(img, r.image_length) : 0;
    pci::write32(a, 0x30, raw);                         // write 2 of 2: restore what we found
    ++g_cfg_writes;
    report("ROM via ROM BAR", r, h);
    const uint16_t v = pci::read16(a, pcireg::VENDOR_ID), d = pci::read16(a, pcireg::DEVICE_ID);
    if (r.pcir_ok)
        kprintf("  ROM PCIR IDs %s the device's own IDs (%04x:%04x)\n",
                (r.vendor == v && r.device == d) ? "match" : "DIFFER FROM", v, d);
    if ((pci::read32(a, pcireg::CLASS_REVISION) >> 8) == 0x030000 && !g_primary_hash) g_primary_hash = h;
}
}  // namespace

namespace gpuprobe {
int config_writes() { return g_cfg_writes; }

void inspect(PciAddr a)
{
    const uint32_t cr = pci::read32(a, pcireg::CLASS_REVISION);
    kprintf("%02x:%02x.%x vendor %04x device %04x class %06x rev %02x subsystem %04x:%04x\n", a.bus, a.dev, a.fn,
            pci::read16(a, pcireg::VENDOR_ID), pci::read16(a, pcireg::DEVICE_ID), cr >> 8, cr & 0xFF,
            pci::read16(a, 0x2C), pci::read16(a, 0x2E));
    kprintf("  command 0x%04x status 0x%04x header type 0x%02x interrupt pin %u line %u\n",
            pci::read16(a, pcireg::COMMAND), pci::read16(a, pcireg::STATUS), pci::read8(a, pcireg::HEADER_TYPE),
            pci::read8(a, pcireg::INTERRUPT_PIN), pci::read8(a, pcireg::INTERRUPT_LINE));
    bars(a);
    caps(a);
    expansion_rom(a);
}

void legacy_shadow()
{
    const volatile uint8_t* img = reinterpret_cast<const volatile uint8_t*>(0xC0000);
    const rom::Info r = rom::parse(img, 128 * 1024);
    const uint32_t h = r.header_ok && r.image_length ? rom::fnv1a(img, r.image_length) : 0;
    kprintf("legacy VGA BIOS shadow at 0xC0000 (memory reads only)\n");
    report("shadow copy", r, h);
    if (g_primary_hash)
        kprintf("  shadow copy %s the primary VGA device's ROM image\n", h == g_primary_hash ? "is identical to" : "DIFFERS FROM");
}
}  // namespace gpuprobe
