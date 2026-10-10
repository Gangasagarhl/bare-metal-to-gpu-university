// msi.h - DR302 F4-08: message-signalled interrupts (MSI and MSI-X) for PCI drivers.
// Capability offsets: PCI Local Bus Specification ("MSI Capability Structure") and PCI
// Express Base Specification ("MSI-X Capability and Table Structure"); checked against
// Linux's <linux/pci_regs.h> by msicheck.cpp. Message format: Intel SDM Vol. 3
// ("Message Signalled Interrupts"), from memory, pending verification.
#pragma once
#include <stdint.h>
#include "../F4-02/pci.h"

namespace msireg {
// MSI capability (ID 0x05)
constexpr uint16_t MSI_FLAGS = 0x02, MSI_ADDR_LO = 0x04, MSI_ADDR_HI = 0x08;
constexpr uint16_t MSI_DATA_32 = 0x08, MSI_DATA_64 = 0x0C, MSI_MASK_64 = 0x10;
constexpr uint16_t MSI_ENABLE = 0x0001, MSI_QMASK = 0x000E, MSI_QSIZE = 0x0070;
constexpr uint16_t MSI_64BIT = 0x0080, MSI_MASKBIT = 0x0100;
// MSI-X capability (ID 0x11) and its table in a BAR
constexpr uint16_t MSIX_FLAGS = 0x02, MSIX_TABLE = 0x04, MSIX_PBA = 0x08;
constexpr uint16_t MSIX_QSIZE = 0x07FF, MSIX_MASKALL = 0x4000, MSIX_ENABLE = 0x8000;
constexpr uint32_t MSIX_BIR = 0x7, MSIX_OFFSET = 0xFFFFFFF8u;
constexpr uint32_t ENTRY_SIZE = 16, ENTRY_ADDR_LO = 0x0, ENTRY_ADDR_HI = 0x4, ENTRY_DATA = 0x8,
                   ENTRY_CTRL = 0xC, ENTRY_MASKED = 0x1;
}

// The x86 message: address 0xFEE00000 | destination APIC ID << 12; data = vector
// (fixed delivery, edge trigger). Physical destination mode, no redirection hint.
struct MsiMessage { uint32_t addr_lo, addr_hi, data; };
MsiMessage msi_message(uint8_t vector, uint32_t apic_id);

namespace msi {
// Plain MSI with one vector. Returns false if the function has no MSI capability.
bool enable(PciAddr a, uint8_t vector);
void disable(PciAddr a);
void print(PciAddr a);                       // the capability as the device now holds it
}

namespace msix {
struct Table {
    uintptr_t table = 0, pba = 0;            // MMIO addresses (paging is off)
    uint16_t size = 0;                       // number of entries
    uint16_t cap = 0;                        // capability offset in configuration space
};
// Finds the capability, sizes the BARs it names, enables MSI-X with every entry masked.
bool setup(PciAddr a, Table& t);
void program(const Table& t, uint16_t entry, uint8_t vector);  // writes the message, leaves mask
void mask(const Table& t, uint16_t entry, bool masked);
bool pending(const Table& t, uint16_t entry);                   // the entry's PBA bit
void print_entry(const Table& t, uint16_t entry);
}
