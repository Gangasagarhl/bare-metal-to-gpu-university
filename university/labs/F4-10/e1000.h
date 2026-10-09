// e1000.h - DR302 F4-10: driver for the Intel 82540EM Gigabit Ethernet controller
// ("e1000" in QEMU). Register offsets, bits and the legacy descriptor formats were written
// from the author's memory of the PCI/PCI-X Family of Gigabit Ethernet Controllers
// Software Developer's Manual (8254x), sections "General Initialization", "Receive
// Descriptor Format", "Transmit Descriptor Format" and the register summary. Not opened in
// this build: confirmed only by QEMU's e1000 model behaving as expected (see F4-10).
#pragma once
#include <stdint.h>
#include "../F4-02/pci.h"

namespace e1000reg {
constexpr uint32_t CTRL = 0x0000, STATUS = 0x0008, EERD = 0x0014, ICR = 0x00C0, ICS = 0x00C8,
                   IMS = 0x00D0, IMC = 0x00D8, RCTL = 0x0100, TCTL = 0x0400, TIPG = 0x0410,
                   RDBAL = 0x2800, RDBAH = 0x2804, RDLEN = 0x2808, RDH = 0x2810, RDT = 0x2818,
                   TDBAL = 0x3800, TDBAH = 0x3804, TDLEN = 0x3808, TDH = 0x3810, TDT = 0x3818,
                   MTA = 0x5200, RAL0 = 0x5400, RAH0 = 0x5404,
                   MPC = 0x4010, RNBC = 0x40A0;   // statistics: missed packets, no buffers (clear on read)
constexpr uint32_t CTRL_ASDE = 1u << 5, CTRL_SLU = 1u << 6, CTRL_RST = 1u << 26;
constexpr uint32_t STATUS_LU = 1u << 1;
constexpr uint32_t EERD_START = 1u << 0, EERD_DONE = 1u << 4;
constexpr uint32_t RCTL_EN = 1u << 1, RCTL_BAM = 1u << 15, RCTL_SECRC = 1u << 26;   // BSIZE 00 = 2048
constexpr uint32_t TCTL_EN = 1u << 1, TCTL_PSP = 1u << 3;
constexpr uint32_t ICR_TXDW = 1u << 0, ICR_LSC = 1u << 2, ICR_RXT0 = 1u << 7;
constexpr uint8_t RXD_DD = 1u << 0, RXD_EOP = 1u << 1;
constexpr uint8_t TXD_EOP = 1u << 0, TXD_IFCS = 1u << 1, TXD_RS = 1u << 3, TXD_DD = 1u << 0;
}

namespace e1000 {
struct RxDesc { uint64_t addr; uint16_t length, checksum; uint8_t status, errors; uint16_t special; };
struct TxDesc { uint64_t addr; uint16_t length; uint8_t cso, cmd, status, css; uint16_t special; };
static_assert(sizeof(RxDesc) == 16 && sizeof(TxDesc) == 16, "legacy descriptors are 16 bytes");

bool find(PciAddr& out);                 // 8086:100e
bool init(PciAddr a);                    // reset, MAC, rings, receive and transmit on
const uint8_t* mac();
bool link_up();
bool send(const void* frame, uint16_t len);          // copies into a transmit buffer
// Returns the length of the next received frame copied into buf (0 = nothing waiting).
uint16_t receive(void* buf, uint16_t max);
uint32_t read(uint32_t reg);
void write(uint32_t reg, uint32_t v);
uint16_t eeprom_word(uint8_t addr);
}
