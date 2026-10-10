// virtio_net.h - DR301 F4-05: a minimal virtio network driver (one RX and one TX queue).
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../F4-02/pci.h"

namespace vnet {
int init(PciAddr a);
const uint8_t* mac();
void send(const uint8_t* frame, size_t len);              // copies, waits until sent
size_t receive(uint8_t* frame, size_t cap, uint32_t spins); // 0 if nothing arrived in time
}
