// virtio_blk.h - DR301 F4-05: a virtio block driver with up to 32 requests in flight.
#pragma once
#include <stdint.h>
#include "../F4-02/pci.h"

namespace vblk {
int init(PciAddr a);                       // 0 or a negative error
uint64_t capacity_sectors();               // in 512-byte sectors, from the device config
// Start one 4 KiB request (8 sectors) using slot 'slot' (0..31); 'data' is the slot's buffer.
uint8_t* slot_buffer(int slot);
void start(int slot, bool write, uint64_t sector);
// Wait for any request to finish; returns its slot and puts the device's status in 'status'.
int wait_any(uint8_t& status);
constexpr int SLOTS = 32;
}
