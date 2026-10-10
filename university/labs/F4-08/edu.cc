// edu.cc - DR302 F4-08: the edu device, MMIO through BAR0.
#include "edu.h"
#include "../F4-01/kbase.h"

namespace {
PciAddr g_found{0xFF, 0, 0};
uintptr_t g_bar = 0;
void probe(PciAddr a)
{
    if (pci::read16(a, pcireg::VENDOR_ID) == 0x1234 && pci::read16(a, pcireg::DEVICE_ID) == 0x11E8)
        g_found = a;
}
}  // namespace

namespace edu {
bool find(PciAddr& out)
{
    pci::enumerate(probe);
    out = g_found;
    return g_found.bus != 0xFF;
}

void init(PciAddr a)
{
    Bar bars[6];
    pci::size_bars(a, bars, 6);
    g_bar = static_cast<uintptr_t>(bars[0].addr);
    pci::enable(a, pcireg::CMD_MEMORY | pcireg::CMD_MASTER);
}

uint32_t reg(uint32_t off) { return mmio_read<uint32_t>(g_bar + off); }
void set(uint32_t off, uint32_t v) { mmio_write<uint32_t>(g_bar + off, v); }
void set64(uint32_t off, uint64_t v)
{
    mmio_write<uint32_t>(g_bar + off, static_cast<uint32_t>(v));
    mmio_write<uint32_t>(g_bar + off + 4, static_cast<uint32_t>(v >> 32));
}

void dma_start(uint64_t iova, uint32_t bytes, bool to_ram, bool irq)
{
    set64(DMA_SRC, to_ram ? DEV_BUFFER : iova);
    set64(DMA_DST, to_ram ? iova : DEV_BUFFER);
    set64(DMA_COUNT, bytes);
    set(DMA_CMD, DMA_START | (to_ram ? DMA_TO_RAM : 0) | (irq ? DMA_IRQ : 0));
}

bool dma_busy() { return reg(DMA_CMD) & DMA_START; }
}  // namespace edu
