// msi.cc - DR302 F4-08: MSI and MSI-X programming.
#include "msi.h"
#include "intr.h"
#include "../F4-01/kbase.h"

using namespace msireg;

MsiMessage msi_message(uint8_t vector, uint32_t apic_id)
{
    // Address bits 31:20 = 0xFEE, 19:12 = destination ID; bit 3 RH = 0, bit 2 DM = 0.
    // Data bits 7:0 = vector, 10:8 = delivery mode 000 (fixed), bit 15 = 0 (edge).
    return MsiMessage{0xFEE00000u | (apic_id << 12), 0, vector};
}

namespace msi {
bool enable(PciAddr a, uint8_t vector)
{
    const uint16_t c = pci::find_cap(a, pcireg::CAP_MSI);
    if (!c) return false;
    uint16_t flags = pci::read16(a, c + MSI_FLAGS);
    const MsiMessage m = msi_message(vector, intr::lapic_id());
    pci::write32(a, c + MSI_ADDR_LO, m.addr_lo);
    if (flags & MSI_64BIT) {
        pci::write32(a, c + MSI_ADDR_HI, m.addr_hi);
        pci::write16(a, c + MSI_DATA_64, static_cast<uint16_t>(m.data));
    } else {
        pci::write16(a, c + MSI_DATA_32, static_cast<uint16_t>(m.data));
    }
    flags = static_cast<uint16_t>((flags & ~MSI_QSIZE) | MSI_ENABLE);   // one vector (2^0)
    pci::write16(a, c + MSI_FLAGS, flags);
    // With MSI on, the legacy INTx pin must stay quiet: set "interrupt disable".
    pci::enable(a, pcireg::CMD_INTX_DISABLE | pcireg::CMD_MASTER);
    return true;
}

void disable(PciAddr a)
{
    const uint16_t c = pci::find_cap(a, pcireg::CAP_MSI);
    if (c) pci::write16(a, c + MSI_FLAGS, pci::read16(a, c + MSI_FLAGS) & ~MSI_ENABLE);
}

void print(PciAddr a)
{
    const uint16_t c = pci::find_cap(a, pcireg::CAP_MSI);
    if (!c) { kprintf("msi: %02x:%02x.%x has no MSI capability\n", a.bus, a.dev, a.fn); return; }
    const uint16_t f = pci::read16(a, c + MSI_FLAGS);
    const bool is64 = f & MSI_64BIT;
    kprintf("msi: cap at 0x%02x control 0x%04x (enable=%u, 64-bit=%u, per-vector mask=%u, "
            "vectors capable=%u)\n", c, f, f & MSI_ENABLE, is64 ? 1 : 0, (f & MSI_MASKBIT) ? 1 : 0,
            1u << ((f & MSI_QMASK) >> 1));
    kprintf("msi: address 0x%08x%08x data 0x%04x\n", is64 ? pci::read32(a, c + MSI_ADDR_HI) : 0,
            pci::read32(a, c + MSI_ADDR_LO), pci::read16(a, c + (is64 ? MSI_DATA_64 : MSI_DATA_32)));
}
}  // namespace msi

namespace msix {
bool setup(PciAddr a, Table& t)
{
    const uint16_t c = pci::find_cap(a, pcireg::CAP_MSIX);
    if (!c) return false;
    Bar bars[6];
    pci::size_bars(a, bars, 6);
    const uint32_t tab = pci::read32(a, c + MSIX_TABLE), pba = pci::read32(a, c + MSIX_PBA);
    t.cap = c;
    t.size = static_cast<uint16_t>((pci::read16(a, c + MSIX_FLAGS) & MSIX_QSIZE) + 1);
    t.table = static_cast<uintptr_t>(bars[tab & MSIX_BIR].addr) + (tab & MSIX_OFFSET);
    t.pba = static_cast<uintptr_t>(bars[pba & MSIX_BIR].addr) + (pba & MSIX_OFFSET);
    pci::enable(a, pcireg::CMD_MEMORY | pcireg::CMD_MASTER | pcireg::CMD_INTX_DISABLE);
    // Enable with "mask all" first, mask every entry, then drop the function mask.
    pci::write16(a, c + MSIX_FLAGS, MSIX_ENABLE | MSIX_MASKALL);
    for (uint16_t i = 0; i < t.size; ++i) mask(t, i, true);
    pci::write16(a, c + MSIX_FLAGS, MSIX_ENABLE);
    kprintf("msix: cap at 0x%02x, %u entries, table in BAR%u+0x%x, PBA in BAR%u+0x%x\n", c, t.size,
            tab & MSIX_BIR, tab & MSIX_OFFSET, pba & MSIX_BIR, pba & MSIX_OFFSET);
    return true;
}

void program(const Table& t, uint16_t entry, uint8_t vector)
{
    const uintptr_t e = t.table + entry * ENTRY_SIZE;
    const MsiMessage m = msi_message(vector, intr::lapic_id());
    mmio_write<uint32_t>(e + ENTRY_ADDR_LO, m.addr_lo);
    mmio_write<uint32_t>(e + ENTRY_ADDR_HI, m.addr_hi);
    mmio_write<uint32_t>(e + ENTRY_DATA, m.data);
}

void mask(const Table& t, uint16_t entry, bool masked)
{
    const uintptr_t ctrl = t.table + entry * ENTRY_SIZE + ENTRY_CTRL;
    const uint32_t v = mmio_read<uint32_t>(ctrl);
    mmio_write<uint32_t>(ctrl, masked ? v | ENTRY_MASKED : v & ~ENTRY_MASKED);
}

bool pending(const Table& t, uint16_t entry)
{
    return (mmio_read<uint32_t>(t.pba + 4 * (entry / 32)) >> (entry % 32)) & 1;
}

void print_entry(const Table& t, uint16_t entry)
{
    const uintptr_t e = t.table + entry * ENTRY_SIZE;
    kprintf("msix: entry %u address 0x%08x%08x data 0x%08x control 0x%x (%s), PBA bit %u\n", entry,
            mmio_read<uint32_t>(e + ENTRY_ADDR_HI), mmio_read<uint32_t>(e + ENTRY_ADDR_LO),
            mmio_read<uint32_t>(e + ENTRY_DATA), mmio_read<uint32_t>(e + ENTRY_CTRL),
            (mmio_read<uint32_t>(e + ENTRY_CTRL) & ENTRY_MASKED) ? "masked" : "unmasked",
            pending(t, entry) ? 1 : 0);
}
}  // namespace msix
