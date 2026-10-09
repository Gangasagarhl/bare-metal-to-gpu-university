// virtio_mmio.h - F4-26: the virtio-mmio transport (version 2, "modern") and one split
// virtqueue, enough to read a block device synchronously. Register offsets after the OASIS
// "Virtual I/O Device (VIRTIO)" specification, section "Virtio Over MMIO" (title only,
// pending verification); the lab run reading a known sector checks them against QEMU.
#pragma once
#include <cstdint>
#include "fdt.h"

namespace vmmio {

// Lists every virtio,mmio slot of the devicetree with its magic, version and device ID.
// Returns the base address of the first slot holding device_id, or 0.
uint64_t scan(const fdt::Blob& dt, uint32_t want_device_id);

// virtio-blk over virtio-mmio: reset, negotiate VERSION_1, set up queue 0.
// Returns false (and says why) if the device is not a version 2 device.
bool blk_init(uint64_t base);
uint64_t blk_capacity();                       // in 512-byte sectors
bool blk_read(uint64_t sector, uint8_t* buf);  // one sector, waits by polling the used ring

} // namespace vmmio
