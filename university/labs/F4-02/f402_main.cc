// f402_main.cc - DR301 F4-02: enumerate every PCI function, size its BARs, walk its
// capabilities, register it with the driver model, and bind drivers by ID or class.
#include "../F4-01/driver.h"
#include "acpi.h"
#include "kbase.h"
#include "pci.h"

namespace {
const char* cap_name(uint8_t id)
{
    switch (id) {
    case pcireg::CAP_PM: return "PM";
    case pcireg::CAP_MSI: return "MSI";
    case pcireg::CAP_VENDOR: return "vendor";
    case pcireg::CAP_EXP: return "PCIe";
    case pcireg::CAP_MSIX: return "MSI-X";
    default: return "other";
    }
}

char g_names[48][8];             // "bb:dd.f" strings for the driver model
int g_count = 0;

void report(PciAddr a)
{
    const uint16_t ven = pci::read16(a, pcireg::VENDOR_ID), dev = pci::read16(a, pcireg::DEVICE_ID);
    const uint32_t cr = pci::read32(a, pcireg::CLASS_REVISION);
    const uint8_t cls = cr >> 24, sub = (cr >> 16) & 0xFF, pif = (cr >> 8) & 0xFF;
    const uint8_t hdr = pci::read8(a, pcireg::HEADER_TYPE);
    kprintf("PCI %02x:%02x.%x %04x:%04x class %02x.%02x.%02x hdr %u", a.bus, a.dev, a.fn, ven, dev,
            cls, sub, pif, hdr & pcireg::HDR_MASK);
    if ((hdr & pcireg::HDR_MASK) == pcireg::HDR_BRIDGE)
        kprintf(" bridge to bus %u..%u", pci::read8(a, pcireg::SECONDARY_BUS), pci::read8(a, pcireg::SUBORDINATE_BUS));
    const uint8_t pin = pci::read8(a, pcireg::INTERRUPT_PIN);
    if (pin) kprintf(" pin %c line %u", 'A' + pin - 1, pci::read8(a, pcireg::INTERRUPT_LINE));
    kprintf("\n");

    Device d{};
    if ((hdr & pcireg::HDR_MASK) <= pcireg::HDR_BRIDGE) {
        Bar bars[6];
        pci::trace_bars = a.bus == 0 && (a.dev == 2 || a.dev == 3);   // e1000 and NVMe only
        pci::size_bars(a, bars, (hdr & pcireg::HDR_MASK) == pcireg::HDR_BRIDGE ? 2 : 6);
        for (int i = 0; i < 6; ++i) {
            const Bar& b = bars[i];
            if (b.size == 0) continue;
            kprintf("  BAR%d %s 0x%lx size 0x%lx%s\n", i, b.io ? "io   " : (b.is64 ? "mem64" : "mem32"),
                    b.addr, b.size, b.prefetch ? " prefetchable" : "");
            if (b.io && !d.io_len) { d.io_base = static_cast<uint16_t>(b.addr); d.io_len = static_cast<uint16_t>(b.size); }
            if (!b.io && !d.mmio_len) { d.mmio_base = b.addr; d.mmio_len = b.size; }
        }
    }
    // capabilities: the classic list in the first 256 bytes ...
    if (pci::read16(a, pcireg::STATUS) & pcireg::STATUS_CAP_LIST) {
        kprintf("  caps:");
        uint8_t p = pci::read8(a, pcireg::CAP_PTR) & 0xFC;
        for (int guard = 0; p && guard < 48; ++guard) {
            const uint8_t id = pci::read8(a, p);
            kprintf(" %02x(%s)@0x%02x", id, cap_name(id), p);
            p = pci::read8(a, static_cast<uint16_t>(p + 1)) & 0xFC;
        }
        kprintf("\n");
    }
    // ... and the PCI Express extended list from 0x100, reachable only through ECAM.
    if (pci::ecam_active() && pci::find_cap(a, pcireg::CAP_EXP)) {
        uint16_t p = pcireg::EXT_CAP_START;
        uint32_t h = pci::read32(a, p);
        if (h != 0 && h != 0xFFFFFFFF) {
            kprintf("  ext caps:");
            for (int guard = 0; p && guard < 64; ++guard) {
                h = pci::read32(a, p);
                kprintf(" %04x(v%u)@0x%03x", h & 0xFFFF, (h >> 16) & 0xF, p);
                p = static_cast<uint16_t>((h >> 20) & 0xFFC);
            }
            kprintf("\n");
        }
    }
    if (g_count < 48) {
        char* n = g_names[g_count++];
        const char* hex = "0123456789abcdef";
        n[0] = hex[a.bus >> 4]; n[1] = hex[a.bus & 15]; n[2] = ':';
        n[3] = hex[a.dev >> 4]; n[4] = hex[a.dev & 15]; n[5] = '.'; n[6] = hex[a.fn]; n[7] = 0;
        d.name = n;
        d.bus = BusKind::Pci;
        d.vendor = ven;
        d.device = dev;
        d.cls = cls; d.subcls = sub; d.progif = pif;
        d.irq = pin ? pci::read8(a, pcireg::INTERRUPT_LINE) : 0xFF;
        Device& reg = dm::add_device(d);
        reg.priv = nullptr;
    }
}

PciAddr addr_of(const Device& d)
{
    auto h = [](char c) { return static_cast<uint8_t>(c <= '9' ? c - '0' : c - 'a' + 10); };
    return PciAddr{static_cast<uint8_t>(h(d.name[0]) * 16 + h(d.name[1])),
                   static_cast<uint8_t>(h(d.name[3]) * 16 + h(d.name[4])), h(d.name[6])};
}

// Placeholder drivers: they claim the device, turn on memory decoding and bus mastering,
// and stop. F4-05, F4-06 and F4-07 replace them with real ones.
int stub_probe(Device& d)
{
    pci::enable(addr_of(d), pcireg::CMD_MEMORY | pcireg::CMD_IO | pcireg::CMD_MASTER);
    return 0;
}
constexpr uint32_t CLASS_ALL = 0xFFFFFF, CLASS_NO_PIF = 0xFFFF00;
constexpr MatchId virtio_ids[] = {{BusKind::Pci, nullptr, 0x1AF4, 0xFFFF, 0, 0}};
constexpr MatchId ahci_ids[] = {{BusKind::Pci, nullptr, 0xFFFF, 0xFFFF, 0x010601, CLASS_ALL}};
constexpr MatchId nvme_ids[] = {{BusKind::Pci, nullptr, 0xFFFF, 0xFFFF, 0x010802, CLASS_ALL}};
constexpr MatchId xhci_ids[] = {{BusKind::Pci, nullptr, 0xFFFF, 0xFFFF, 0x0C0330, CLASS_ALL}};
constexpr MatchId e1000_ids[] = {{BusKind::Pci, nullptr, 0x8086, 0x100E, 0, 0}};
constexpr MatchId hda_ids[] = {{BusKind::Pci, nullptr, 0xFFFF, 0xFFFF, 0x040300, CLASS_NO_PIF}};
Driver drivers[] = {
    {"virtio-pci", virtio_ids, 1, stub_probe, nullptr},
    {"ahci", ahci_ids, 1, stub_probe, nullptr},
    {"nvme", nvme_ids, 1, stub_probe, nullptr},
    {"xhci", xhci_ids, 1, stub_probe, nullptr},
    {"e1000", e1000_ids, 1, stub_probe, nullptr},
    {"hda", hda_ids, 1, stub_probe, nullptr},
};
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t mbi)
{
    serial_init();
    kprintf("F4-02 kernel: magic=0x%x\n", magic);
    uint64_t base = 0;
    uint8_t b0 = 0, b1 = 0;
    if (acpi::init() && acpi::ecam(base, b0, b1)) {
        pci::use_ecam(base, b0, b1);
        kprintf("pci: ECAM at 0x%lx for buses %u..%u (from MCFG)\n", base, b0, b1);
    } else {
        kprintf("pci: no MCFG table, using configuration ports 0xCF8/0xCFC\n");
    }
    pci::enumerate(report);
    for (Driver& drv : drivers) dm::register_driver(drv);
    dm::bind_all();
    kprintf("== driver binding ==\n");
    for (size_t i = 0; i < dm::device_count(); ++i) {
        const Device& d = dm::device_at(i);
        kprintf("%s %04x:%04x -> %s\n", d.name, d.vendor, d.device, d.driver ? d.driver->name : "(no driver)");
    }
    kprintf("F4-02 done\n");
    // With "hold" on the Multiboot command line, stay alive so the lab script can ask
    // QEMU's monitor for its own PCI listing; otherwise end the run with "pass".
    const auto* info = reinterpret_cast<const uint32_t*>(mbi);
    if ((info[0] & 0x4) && kstrcmp(reinterpret_cast<const char*>(info[4]), "hold") == 0)
        for (;;) asm volatile("hlt");
    qemu_exit(0x10);
}
