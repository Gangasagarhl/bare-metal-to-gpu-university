// ahci_regs.h - DR301 F4-06: AHCI register offsets and bits used by ahci.cc.
// Written from the author's memory of the Serial ATA AHCI specification (sections on HBA
// memory registers and port registers); no independent source exists in the build
// container. Confirmed only by QEMU 8.2.2's ich9-ahci model behaving as expected and by
// QEMU's own trace of the register names it saw (ahci_trace.out). See the unverified box.
#pragma once
#include <stdint.h>

namespace hba {        // generic host control, at the start of ABAR (BAR5)
constexpr uint32_t CAP = 0x00, GHC = 0x04, IS = 0x08, PI = 0x0C, VS = 0x10;
constexpr uint32_t GHC_HR = 1u << 0, GHC_IE = 1u << 1, GHC_AE = 1u << 31;
constexpr uint32_t CAP_NP_MASK = 0x1F, CAP_NCS_SHIFT = 8, CAP_S64A = 1u << 31, CAP_SNCQ = 1u << 30;
}
namespace port {       // per port, at ABAR + 0x100 + 0x80 * port
constexpr uint32_t CLB = 0x00, CLBU = 0x04, FB = 0x08, FBU = 0x0C, IS = 0x10, IE = 0x14,
                   CMD = 0x18, TFD = 0x20, SIG = 0x24, SSTS = 0x28, SCTL = 0x2C, SERR = 0x30,
                   SACT = 0x34, CI = 0x38;
constexpr uint32_t CMD_ST = 1u << 0, CMD_SUD = 1u << 1, CMD_POD = 1u << 2, CMD_FRE = 1u << 4,
                   CMD_FR = 1u << 14, CMD_CR = 1u << 15;
constexpr uint32_t IS_DHRS = 1u << 0, IS_TFES = 1u << 30;
constexpr uint32_t TFD_ERR = 1u << 0, TFD_DRQ = 1u << 3, TFD_BSY = 1u << 7;
constexpr uint32_t SSTS_DET_MASK = 0xF, DET_PRESENT = 3;   // device present, link up
constexpr uint32_t SIG_ATA = 0x00000101, SIG_ATAPI = 0xEB140101;
}
namespace ata {
constexpr uint8_t FIS_REG_H2D = 0x27;
constexpr uint8_t IDENTIFY = 0xEC, READ_DMA_EXT = 0x25, WRITE_DMA_EXT = 0x35, FLUSH_EXT = 0xEA;
}

struct CmdHeader {     // one of 32 slots in the command list
    uint16_t flags;    // bits 4:0 FIS length in dwords, bit 6 write, bit 10 clear busy on R_OK
    uint16_t prdtl;    // number of PRD entries
    volatile uint32_t prdbc;   // bytes transferred (written by the HBA)
    uint32_t ctba, ctbau;      // command table address (128-byte aligned)
    uint32_t reserved[4];
};
struct Prd { uint32_t dba, dbau, reserved, dbc; };   // dbc: byte count - 1, bit 31 interrupt
struct CmdTable {
    uint8_t cfis[64];  // the command FIS
    uint8_t acmd[16];  // ATAPI command (unused)
    uint8_t reserved[48];
    Prd prdt[8];
};
static_assert(sizeof(CmdHeader) == 32 && sizeof(Prd) == 16, "AHCI structure sizes");
static_assert(sizeof(CmdTable) == 0x80 + 8 * 16, "PRDT starts at offset 0x80");
