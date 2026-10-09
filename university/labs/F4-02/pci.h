// pci.h - DR301 F4-02: PCI configuration space access, enumeration and BAR sizing.
// Offsets of the type 0/1 configuration header (PCI Local Bus Specification,
// "Configuration Space"); checked against Linux's <linux/pci_regs.h> by pcicheck.cpp.
#pragma once
#include <stdint.h>

namespace pcireg {
constexpr uint16_t VENDOR_ID = 0x00, DEVICE_ID = 0x02, COMMAND = 0x04, STATUS = 0x06;
constexpr uint16_t CLASS_REVISION = 0x08, HEADER_TYPE = 0x0E, BAR0 = 0x10;
constexpr uint16_t PRIMARY_BUS = 0x18, SECONDARY_BUS = 0x19, SUBORDINATE_BUS = 0x1A;
constexpr uint16_t CAP_PTR = 0x34, INTERRUPT_LINE = 0x3C, INTERRUPT_PIN = 0x3D;
constexpr uint16_t CMD_IO = 0x1, CMD_MEMORY = 0x2, CMD_MASTER = 0x4, CMD_INTX_DISABLE = 0x400;
constexpr uint16_t STATUS_CAP_LIST = 0x10;
constexpr uint8_t HDR_MASK = 0x7F, HDR_MULTIFUNCTION = 0x80, HDR_BRIDGE = 0x01;
constexpr uint32_t BAR_IO = 0x1, BAR_TYPE_MASK = 0x6, BAR_TYPE_64 = 0x4, BAR_PREFETCH = 0x8;
constexpr uint8_t CAP_PM = 0x01, CAP_MSI = 0x05, CAP_VENDOR = 0x09, CAP_EXP = 0x10, CAP_MSIX = 0x11;
constexpr uint16_t EXT_CAP_START = 0x100;
}

struct PciAddr { uint8_t bus, dev, fn; };

struct Bar {
    uint64_t addr;               // 0 if the BAR is unused
    uint64_t size;
    bool io, is64, prefetch;
};

namespace pci {
void use_ecam(uint64_t base, uint8_t bus_start, uint8_t bus_end);  // otherwise ports 0xCF8/0xCFC
bool ecam_active();
uint32_t read32(PciAddr a, uint16_t off);
uint16_t read16(PciAddr a, uint16_t off);
uint8_t read8(PciAddr a, uint16_t off);
void write32(PciAddr a, uint16_t off, uint32_t v);
void write16(PciAddr a, uint16_t off, uint16_t v);
int size_bars(PciAddr a, Bar out[6], int nbars);  // 6 BARs in a type 0 header, 2 in type 1
// Calls fn(addr) for every function present, descending through PCI-to-PCI bridges.
void enumerate(void (*fn)(PciAddr a));
// Capability walk: returns the offset of capability 'id', or 0.
uint16_t find_cap(PciAddr a, uint8_t id);
void enable(PciAddr a, uint16_t command_bits);  // set bits in the command register
extern bool trace_bars;                         // print the raw sizing reads (worked example)
}
