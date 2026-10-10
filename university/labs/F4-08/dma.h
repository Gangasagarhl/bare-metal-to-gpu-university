// dma.h - DR302 F4-08: the DMA mapping API every DR302 driver uses.
// A driver never hands a device a physical address directly; it asks for a mapping and
// gets back the address the DEVICE must use (an I/O virtual address, IOVA). Without an
// IOMMU the IOVA equals the physical address (paging is off in the lab kernel).
#pragma once
#include <stdint.h>
#include "../F4-02/pci.h"

namespace dma {
enum class Mode { Identity, Iommu };
void use_identity();                             // no remapping: IOVA = physical
bool use_iommu();                                // VT-d on; false if the machine has none
Mode mode();
void attach(PciAddr dev);                        // give the device its own domain (IOMMU mode)
// Maps [buf, buf + bytes) for 'dev'; 'device_writes' = the device may write there.
// Returns the IOVA of buf. In IOMMU mode IOVAs come from a per-device window that
// starts at 8 MiB, so they differ from the physical addresses on purpose.
uint64_t map(PciAddr dev, const void* buf, uint32_t bytes, bool device_writes);
void unmap(PciAddr dev, uint64_t iova, uint32_t bytes);   // and invalidates the IOTLB
}
