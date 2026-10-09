// virtio_gpu.h - DR405 F4-43: a 2D virtio-gpu driver on the DR301 virtio transport (F4-05).
#pragma once
#include <stdint.h>
#include "../F4-02/pci.h"
#include "vgpu_proto.h"

namespace vgpu {
int init(PciAddr a);                                   // transport, two queues, DRIVER_OK
uint32_t num_scanouts();
uint32_t events();                                     // config.events_read
void clear_events(uint32_t bits);
// Control queue: each call sends one request and waits for its response.
// The return value is the response type (RESP_OK_... or RESP_ERR_...).
uint32_t display_info(vgp::RespDisplayInfo& out);
uint32_t edid(uint32_t scanout, vgp::RespEdid& out);
uint32_t create_2d(uint32_t id, uint32_t format, uint32_t w, uint32_t h);
uint32_t attach_backing(uint32_t id, const void* mem, uint32_t bytes);
uint32_t set_scanout(uint32_t scanout, uint32_t id, uint32_t w, uint32_t h);
uint32_t transfer_2d(uint32_t id, vgp::Rect r, uint32_t stride_bytes);
uint32_t flush(uint32_t id, vgp::Rect r);
// Cursor queue: the device sends no data back, only the used-ring entry.
void update_cursor(uint32_t scanout, uint32_t id, uint32_t x, uint32_t y, uint32_t hot_x, uint32_t hot_y);
void move_cursor(uint32_t scanout, uint32_t x, uint32_t y);
const char* resp_name(uint32_t t);
}  // namespace vgpu
