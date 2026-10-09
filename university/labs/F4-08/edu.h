// edu.h - DR302 F4-08: driver for QEMU's "edu" teaching device (PCI 1234:11e8).
// The register map is QEMU's own documentation of the device (docs/specs/edu.rst in the
// QEMU source, tier 2), written from memory and confirmed only by the device's behaviour.
#pragma once
#include <stdint.h>
#include "../F4-02/pci.h"

namespace edu {
constexpr uint32_t ID = 0x00, LIVENESS = 0x04, FACTORIAL = 0x08, STATUS = 0x20, IRQ_STATUS = 0x24,
                   IRQ_RAISE = 0x60, IRQ_ACK = 0x64, DMA_SRC = 0x80, DMA_DST = 0x88, DMA_COUNT = 0x90,
                   DMA_CMD = 0x98;
constexpr uint32_t STATUS_COMPUTING = 0x01, STATUS_IRQ_FACT = 0x80;
constexpr uint32_t DMA_START = 0x1, DMA_TO_RAM = 0x2, DMA_IRQ = 0x4;
constexpr uint64_t DEV_BUFFER = 0x40000;         // the device's internal 4 KiB buffer
constexpr uint32_t IRQ_DMA_DONE = 0x100;

bool find(PciAddr& out);
void init(PciAddr a);                            // BAR0, memory decode, bus master
uint32_t reg(uint32_t off);
void set(uint32_t off, uint32_t v);
void set64(uint32_t off, uint64_t v);
// Starts a DMA between RAM address 'iova' (as the device sees it) and the device buffer.
void dma_start(uint64_t iova, uint32_t bytes, bool to_ram, bool irq);
bool dma_busy();
}
