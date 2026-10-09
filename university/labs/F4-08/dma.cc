// dma.cc - DR302 F4-08: identity or VT-d backed DMA mappings.
#include "dma.h"
#include "vtd.h"
#include "../F4-01/kbase.h"

namespace {
dma::Mode g_mode = dma::Mode::Identity;
uint64_t g_next_iova[256];                       // per device-function bump allocator
uint16_t g_next_domain = 1;
}

namespace dma {
void use_identity() { g_mode = Mode::Identity; }
bool use_iommu()
{
    if (!vtd::init()) return false;
    g_mode = Mode::Iommu;
    return true;
}
Mode mode() { return g_mode; }

void attach(PciAddr dev)
{
    if (g_mode != Mode::Iommu || g_next_iova[dev.dev << 3 | dev.fn] != 0) return;   // once
    vtd::attach(dev, g_next_domain++);
    g_next_iova[dev.dev << 3 | dev.fn] = 0x00800000;   // 8 MiB: clearly not the kernel's addresses
}

uint64_t map(PciAddr dev, const void* buf, uint32_t bytes, bool device_writes)
{
    const uintptr_t phys = reinterpret_cast<uintptr_t>(buf);
    if (g_mode == Mode::Identity) return phys;
    uint64_t& next = g_next_iova[dev.dev << 3 | dev.fn];
    const uint64_t first = phys & ~0xFFFu, last = (phys + bytes - 1) & ~0xFFFu;
    const uint64_t iova_page = next;
    for (uint64_t p = first; p <= last; p += 4096, next += 4096)
        vtd::map_page(dev, next, p, device_writes);
    next += 4096;                                // an unmapped guard page after every mapping
    return iova_page | (phys & 0xFFF);
}

void unmap(PciAddr dev, uint64_t iova, uint32_t bytes)
{
    if (g_mode == Mode::Identity) return;
    const uint64_t first = iova & ~0xFFFull, last = (iova + bytes - 1) & ~0xFFFull;
    for (uint64_t p = first; p <= last; p += 4096) vtd::unmap_page(dev, p);
    vtd::flush_iotlb();                          // the IOMMU may still cache the old entry
}
}  // namespace dma
